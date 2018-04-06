#include "chatroom_form.h"
#include "audio_hook_form.h"
#include "gui/emoji/emoji_form.h"
#include "module/emoji/richedit_util.h"
#include "shared/ui/ui_menu.h"
#include "richedit.h"
#include <sys\timeb.h>
#include "bypass_live_frontpage.h"
#include "player/player_manager.h"
#include "main_picture_set_form.h"
using namespace nim_comp;

namespace nim_chatroom
{
	const LPTSTR ChatroomForm::kClassName = L"ChatroomForm";
	const int ChatroomForm::kMaxPullRetryTime = 8;
	using namespace ui;

	//is_pull_error_ 暂定为true ,拉流实现此值应为false;
	ChatroomForm::ChatroomForm(__int64 room_id) :
		is_pull_normal_(true)
	{
		room_id_ = room_id;
		has_enter_ = false;
		droptarget_ = NULL;
		time_start_history = 0;
		time_count_ = 0;
		start_time_stamp_ = 0;
		screen_tool_wnd_ = nullptr;
		livestreaming_ = false;
		channel_id_ = 0;
		master_ = false;
		is_audio_interact_ = false;
		is_vedio_interact_ = false;
		is_Camera_ = true;
		need_set_notify_ext_ = false;
		is_open_beauty_ = false;
		audience_ = false;

		gift_rose_count_ = 0;
		gift_ice_count_ = 0;
		gift_cholocate_count_ = 0;
		gift_bear_count_ = 0;

		has_add_creater_ = false;
		is_loading_history_ = false;
		pre_uid_ = "";
	}


	ChatroomForm::~ChatroomForm()
	{
		pre_uid_ = "";
	}
	ui::Control* ChatroomForm::CreateControl(const std::wstring& pstrClass)
	{
		if (pstrClass == _T("BitmapControl"))
		{
			return new ui::CBitmapControl(nullptr);
		}
		else if (pstrClass == _T("Barrage"))
		{
			return new ui::CBarrageControl();
		}
		return NULL;
	}

	std::wstring ChatroomForm::GetSkinFolder()
	{
		return L"chatroom";
	}

	std::wstring ChatroomForm::GetSkinFile()
	{
		return L"chatroom.xml";
	}

	ui::UILIB_RESOURCETYPE ChatroomForm::GetResourceType() const
	{
		return ui::UILIB_RESOURCETYPE::UILIB_FILE;
	}

	std::wstring ChatroomForm::GetZIPFileName() const
	{
		return L"chatroom.zip";
	}

	std::wstring ChatroomForm::GetWindowClassName() const
	{
		return kClassName;
	}

	std::wstring ChatroomForm::GetWindowId() const
	{
		return nbase::Int64ToString16(room_id_);
	}

	UINT ChatroomForm::GetClassStyle() const
	{
		return (UI_CLASSSTYLE_FRAME | CS_DBLCLKS);
	}

	void ChatroomForm::InitWindow()
	{
		m_pRoot->AttachBubbledEvent(ui::kEventAll, nbase::Bind(&ChatroomForm::Notify, this, std::placeholders::_1));
		m_pRoot->AttachBubbledEvent(ui::kEventClick, nbase::Bind(&ChatroomForm::OnClicked, this, std::placeholders::_1));
		m_pRoot->AttachBubbledEvent(ui::kEventSelect, nbase::Bind(&ChatroomForm::OnSelChanged, this, std::placeholders::_1));
		m_pRoot->AttachBubbledEvent(ui::kEventUnSelect, nbase::Bind(&ChatroomForm::OnSelChanged, this, std::placeholders::_1));

		input_edit_ = (RichEdit*)FindControl(L"input_edit");
		input_edit_->SetSelAllOnFocus(false);
		input_edit_->SetEventMask(input_edit_->GetEventMask() | ENM_LINK | ENM_CHANGE);
		input_edit_->AttachReturn(nbase::Bind(&ChatroomForm::OnEditEnter, this, std::placeholders::_1));
		IDropTarget *pdt = input_edit_->GetTxDropTarget();

		ITextServices * text_services = input_edit_->GetTextServices();
		richeditolecallback_ = new nim_comp::IRichEditOleCallbackEx(text_services, std::function<void()>());
		text_services->Release();
		input_edit_->SetOleCallback(richeditolecallback_);
		richeditolecallback_->SetCustomMode(true);

		bypass_prompt_ = (Control*)FindControl(L"bypass_prompt");

		msg_list_ = (RichEdit*)FindControl(L"intercommunicate_list");
		msg_list_->SetSelAllOnFocus(false);
		msg_list_->SetNoSelOnKillFocus(false);

		msg_list_->SetNoCaretReadonly();
		msg_list_->SetEventMask(msg_list_->GetEventMask() | ENM_LINK | ENM_MOUSEEVENTS | ENM_SCROLLEVENTS | ENM_KEYEVENTS);
		msg_list_->SetLimitText(INT_MAX);

		text_services = msg_list_->GetTextServices();
		richeditolecallback_ = new nim_comp::IRichEditOleCallbackEx(text_services, std::function<void()>());
		text_services->Release();
		msg_list_->SetOleCallback(richeditolecallback_);
		richeditolecallback_->SetCustomMode(true);

		name_ = ((ui::Label*)FindControl(L"name"));
		btn_face_ = (ui::CheckBox*)FindControl(L"btn_face");
		online_members_list_ = (ui::ListBox*)FindControl(L"online_members_list");
		empty_members_list_ = FindControl(L"empty_members_list");
		bypass_members_list_ = (ui::ListBox*) FindControl(L"bypass_members_list");

		list_tab_ = (ui::TabBox*)FindControl(L"list_tab");

		bulletin_ = ((ui::RichEdit*)FindControl(L"bulletin"));
		bulletin_->SetLimitText(300);

		ui::Option *option_online_members = (ui::Option*)FindControl(L"option_online_members");
		option_online_members->AttachSelect(nbase::Bind(&ChatroomForm::OnSelOnlineMembers, this, std::placeholders::_1));

		option_bypass_members_ = (ui::Option*)FindControl(L"option_bypass_comunicate");
		option_bypass_members_->AttachSelect(nbase::Bind(&ChatroomForm::OnSelBypassMembers, this, std::placeholders::_1));

		option_bypass_gift_ = (ui::Option*)FindControl(L"option_bypass_gift");

		ls_tab_ = (ui::TabBox*)FindControl(L"ls_tab");
		video_show_ctrl_ = ((ui::CBitmapControl*)FindControl(L"video_screen"));
		bypass_show_ctrl_ = ((ui::CBitmapControl*)FindControl(L"bypass_screen"));
		bypass_show_ctrl_->SetAutoSize(true);
		ls_time_ = ((ui::Label*)FindControl(L"ls_time"));
		paint_video_timer_.Cancel();
		StdClosure task = nbase::Bind(&ChatroomForm::PaintVideo, this);
		nbase::ThreadManager::PostRepeatedTask(kThreadUI, paint_video_timer_.ToWeakCallback(task), nbase::TimeDelta::FromMilliseconds(60));

		btn_start_ls_ = ((ui::Button*)FindControl(L"start_ls"));
		btn_start_ls_->AttachClick(nbase::Bind(&ChatroomForm::OnStartLiveStream, this, std::placeholders::_1));
		btn_stop_ls_ = ((ui::Button*)FindControl(L"stop_ls"));
		btn_stop_ls_->AttachClick(nbase::Bind(&ChatroomForm::OnStopLiveStream, this, std::placeholders::_1));

		btn_camera_set_ = (ui::Button*)FindControl(L"camera_set");
		btn_mic_set_ = (ui::Button*)FindControl(L"mic_set");
		btn_interact_set_ = (ui::Button*)FindControl(L"interact_set");
		btn_model_set_ = (ui::Button*)FindControl(L"model_set"); 
		btn_audiohook_set = (ui::Button*)FindControl(L"audio_hook");

		unregister_cb.Add(nim_ui::HttpManager::GetInstance()->RegDownloadComplete(nbase::Bind(&ChatroomForm::OnHttoDownloadReady, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));

		screen_tool_wnd_ = new nim_comp::ScreenCaptureTool();
		screen_tool_wnd_->SetVideoMng(&video_frame_mng_);
		screen_tool_wnd_->Create(NULL, nim_comp::ScreenCaptureTool::kClassName, WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_MINIMIZEBOX, 0);

		//礼物
		lb_gift_1_ = (ui::Label*)FindControl(L"gift_1");
		lb_gift_2_ = (ui::Label*)FindControl(L"gift_2");
		lb_gift_3_ = (ui::Label*)FindControl(L"gift_3");
		lb_gift_4_ = (ui::Label*)FindControl(L"gift_4");
		lb_gift_1_count_ = (ui::Label*)FindControl(L"gift_1_count");
		lb_gift_2_count_ = (ui::Label*)FindControl(L"gift_2_count");
		lb_gift_3_count_ = (ui::Label*)FindControl(L"gift_3_count");
		lb_gift_4_count_ = (ui::Label*)FindControl(L"gift_4_count");
		gift_png_1_ = (ui::Control*)FindControl(L"gift_png_1");
		gift_png_2_ = (ui::Control*)FindControl(L"gift_png_2");
		gift_png_3_ = (ui::Control*)FindControl(L"gift_png_3");
		gift_png_4_ = (ui::Control*)FindControl(L"gift_png_4");

		gift_box_1_ = (ui::VBox*)FindControl(L"gift_box_1");
		gift_box_2_ = (ui::VBox*)FindControl(L"gift_box_2");
		gift_box_3_ = (ui::VBox*)FindControl(L"gift_box_3");
		gift_box_4_ = (ui::VBox*)FindControl(L"gift_box_4");

		gift_prompt_ = (ui::VBox*)FindControl(L"gift_prompt");
		gift_info_ = (ui::VBox*)FindControl(L"giftinfo");

		bulletin_ = ((ui::RichEdit*)FindControl(L"bulletin"));

		BypassLiveFontPage	*frontPage = (BypassLiveFontPage*)nim_ui::WindowsManager::GetInstance()->GetWindow(nim_chatroom::BypassLiveFontPage::kClassName, nim_chatroom::BypassLiveFontPage::kClassName);
		frontPage->ShowWindow(false);
		frontPage->WindowReset();
	}

	LRESULT ChatroomForm::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (uMsg == WM_SIZE)
		{
			if (wParam == SIZE_RESTORED)
				OnWndSizeMax(false);
			else if (wParam == SIZE_MAXIMIZED)
				OnWndSizeMax(true);
		}
		else if (uMsg == WM_NOTIFY)  // 超链接消息
		{
			if (OnLinkClick(wParam, lParam))
				return 0;
		}
		else if (uMsg == WM_MOUSEWHEEL)
		{
			POINT pt;
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);
			ScreenToClient(GetHWND(), &pt);
			if (msg_list_->GetPos().IsPointIn(pt) && list_tab_->GetCurSel() == 0 && !is_loading_history_)
			{
				int zDelta = (int)(short)HIWORD(wParam);
				if (zDelta > 0)
				{
					if (msg_list_->GetScrollPos().cy == 0)
					{
						is_loading_history_ = true;
						//GetHistorys();
						return 0;
					}
				}
			}
		}

		return __super::HandleMessage(uMsg, wParam, lParam);
	}

	void ChatroomForm::OnFinalMessage(HWND hWnd)
	{
		ChatRoom::Exit(room_id_);
		if (screen_tool_wnd_)
		{
			screen_tool_wnd_->StopCapture();
		}
		if (livestreaming_)
		{
			StopLiveStream();
		}
		{
			if (master_)
			{
				ChatRoom::QueueDropAsync(room_id_, nullptr);
			}
			else
			{
				EN_PLAYER_STATE state = nim_comp::PlayerManager::GetInstance()->GetPlayerState();
				if (state == EN_PLAYER_PLAYING || state == EN_PLAYER_PREPARING)
					nim_comp::PlayerManager::GetInstance()->StopPlay();
			}
		}
		CloseDriver();
		__super::OnFinalMessage(hWnd);

		BypassLiveFontPage* frontPage = (BypassLiveFontPage*)nim_ui::WindowsManager::GetInstance()->GetWindow(nim_chatroom::BypassLiveFontPage::kClassName, nim_chatroom::BypassLiveFontPage::kClassName);
		if (frontPage != NULL)
		{
			frontPage->WindowReset();
			frontPage->ShowWindow();
		}
		
	}

	bool ChatroomForm::Notify(ui::EventArgs * msg)
	{
		return true;
	}

	void ChatroomForm::OnEsc(BOOL &bHandled)
	{
		bHandled = TRUE;
		if (livestreaming_)
		{
			MsgboxCallback cb = [this](MsgBoxRet ret){
				if (ret == MB_YES)
				{
					if (master_)
					{
						UpdateChatroomInfoWithClose(false);
					}
					else
					{
						if (is_audio_interact_ || is_vedio_interact_)
						{
							RequestPopMicLink(nbase::Int64ToString(room_id_), LoginManager::GetInstance()->GetAccount());
							SendSysNotifyMsg(kExitQueue, creater_id_);
							//ChatRoom::Exit(room_id_);
							Close();
						}
					}
					//如果是主播，清空序列
				}
			};

			ShowMsgBox(m_hWnd, L"确定结束直播？", ToWeakCallback(cb), L"提示", L"结束", L"取消");
		}
		else
		{
			MsgboxCallback cb = [this](MsgBoxRet ret){
				if (ret == MB_YES)
				{
					//if (master_)
					//	ChatRoom::QueueDropAsync(room_id_, nullptr);
					if(!master_)
					{
						if (is_audio_interact_ || is_vedio_interact_)
						{
							RequestPopMicLink(nbase::Int64ToString(room_id_), LoginManager::GetInstance()->GetAccount());
							SendSysNotifyMsg(kExitQueue, creater_id_);
						}
					}
					//ChatRoom::Exit(room_id_);
					Close();
				}
			};
			ShowMsgBox(m_hWnd, L"确定退出房间？", ToWeakCallback(cb), L"提示", L"确定", L"取消");
		}
	}

	bool ChatroomForm::OnClicked(ui::EventArgs* param)
	{
		std::wstring name = param->pSender->GetName();
		if (name == L"send")
		{
			OnBtnSend();
		}
		else if (name == L"btn_jsb")
		{
			OnBtnJsb();
		}
		else if (name == L"camera_mode")
		{
			OnLiveStreamInit(true);
		}
		else if (name == L"screen_mode")
		{
			OnLiveStreamInit(false);
		}
		else if (name == L"closebtn_ex")
		{
			BOOL bHandled = FALSE;
			OnEsc(bHandled);
		}
		else if (name == L"main_picture_set")
		{
			MainPictureSetForm *form = nim_ui::WindowsManager::SingletonShow<MainPictureSetForm>(MainPictureSetForm::kClassName);
		}
		else if (name == L"audio_hook")
		{
			//伴音设置
			AudioHookForm *form = nim_ui::WindowsManager::SingletonShow<AudioHookForm>(AudioHookForm::kClassName);
			if (form)
			{
			}
		}
		else if (name == L"camera_set")
		{
			nim_comp::VideoManager::GetInstance()->ShowVideoSetting(true);
		}
		else if (name == L"mic_set")
		{
			nim_comp::VideoManager::GetInstance()->ShowVideoSetting(false);
		}
		else if (name == L"wnd_set")
		{
			Post2UI(nbase::Bind(&ChatroomForm::ShowWindowSelect, this));
		}
		else if (name == L"model_set")
		{
			ui::UiRect rect = btn_model_set_->GetPos();
			ui::CPoint pt;
			ClientToScreen(this->m_hWnd, &pt);
			pt.x += rect.left - 40;
			pt.y += rect.top - 128;
			PopupCaptureChoiceMenu(pt);
		}
		else if (name == L"interact_set")
		{
			if (!master_&&is_pull_normal_)
			{
				ui::UiRect rect = btn_interact_set_->GetPos();
				ui::CPoint pt;
				ClientToScreen(this->m_hWnd, &pt);
				pt.x += rect.left - 18;
				pt.y += rect.top - 85;
				PopupInteractChoiceMenu(pt);
			}
		}
		else if (name == L"option_setting")
		{
			ls_tab_->SelectItem(0);
		}
		else if (name == L"option_ls")
		{
			ls_tab_->SelectItem(1);
		}
		else if (name == L"beauty_set")
		{
			BeautyOperate();
		}
		return true;
	}

	bool ChatroomForm::OnEditEnter(ui::EventArgs* param)
	{
		OnBtnSend();
		return true;
	}

	void ChatroomForm::PopupInteractChoiceMenu(POINT point)
	{
		CMenuWnd* pMenu = new CMenuWnd(NULL);
		STRINGorID xml(L"interact_choice_menu.xml");
		if (living_model_ == kAudio)
		{
			point.y = point.y + 36;
		}
		pMenu->Init(xml, _T("xml"), point);

		CMenuElementUI* vedio_interact_button = (CMenuElementUI*)pMenu->FindControl(L"menu_videointeract");
		CMenuElementUI* audio_interact_button = (CMenuElementUI*)pMenu->FindControl(L"menu_audiointeract");
		Button* btn_vedio_interact = (ui::Button*)pMenu->FindControl(L"videointeract");
		Button* btn_audio_interact = (ui::Button*)pMenu->FindControl(L"audiointeract");
		if (is_vedio_interact_)
		{
			btn_vedio_interact->SetText(L"取消视频互动");
		}
		else
		{
			btn_vedio_interact->SetText(L"视频互动");
		}

		if (is_audio_interact_)
		{
			btn_audio_interact->SetText(L"取消音频互动");
		}
		else
		{
			btn_audio_interact->SetText(L"音频互动");
		}

		vedio_interact_button->AttachSelect(nbase::Bind(&ChatroomForm::OnVideoInteractClick, this, std::placeholders::_1));
		audio_interact_button->AttachSelect(nbase::Bind(&ChatroomForm::OnAudioInteractClick, this, std::placeholders::_1));

		if (living_model_ == kAudio)
		{
			vedio_interact_button->SetVisible(false);
		}
		pMenu->Show(this->GetHWND());
		pMenu->SetFocus(NULL);
	}

	bool ChatroomForm::OnVideoInteractClick(ui::EventArgs* param)
	{
		//正在互动，且互动的模式不是视频，发起的操作无效
		if (is_audio_interact_)
			return true;

		LoginData* logindata = LoginManager::GetInstance()->GetLoginData();
		Json::FastWriter fw;
		Json::Value value_info;
		string uid = LoginManager::GetInstance()->GetAccount();
		std::wstring  nick = nim_ui::UserManager::GetInstance()->GetUserName(uid, false);
		bypass_inact_type_ = kVedio;
		if (!is_vedio_interact_)
		{
			//正在连麦，要取消
			value_info["nick"] = nbase::UTF16ToUTF8(nick);
			value_info["avatar"] = "avatar_default";
			string info = fw.write(value_info);

			Json::Value value;
			value["style"] = kVedio;
			value["state"] = NTESLiveMicStateConnecting;
			value["info"] = info;
			string ext = fw.write(value);
			RequestPushMicLink(nbase::Int64ToString(room_id_), uid, ext, kVedio);
		}
		else
		{
			//连麦者断开连麦
			//先去应用服务器将要移出的人移出
			//退出多人会议室
			//主播收到人员变化，发出全局自定义消息
			RequestPopMicLink(nbase::Int64ToString(room_id_), uid);
			SwitchPullStreamAndInteract(true);
		}
		return true;
	}

	bool ChatroomForm::OnAudioInteractClick(ui::EventArgs* param)
	{
		//正在互动，且互动的模式不是音频，发起的操作无效

		if (is_vedio_interact_)
			return true;

		LoginData* logindata = LoginManager::GetInstance()->GetLoginData();
		Json::FastWriter fw;
		Json::Value value_info;
		string uid = LoginManager::GetInstance()->GetAccount();
		std::wstring  nick = nim_ui::UserManager::GetInstance()->GetUserName(uid, false);
		bypass_inact_type_ = kAudio;
		if (!is_audio_interact_)
		{
			value_info["nick"] = nbase::UTF16ToUTF8(nick);
			value_info["avatar"] = "avatar_default";
			string info = fw.write(value_info);

			Json::Value value;
			value["style"] = kAudio;
			value["state"] = NTESLiveMicStateConnecting;
			value["info"] = info;
			string ext = fw.write(value);
			RequestPushMicLink(nbase::Int64ToString(room_id_), uid, ext, kAudio);
		}
		else
		{
			RequestPopMicLink(nbase::Int64ToString(room_id_), uid);
			SwitchPullStreamAndInteract(true);
		}
		return false;
	}

	void ChatroomForm::SetInteractState(InactionType type)
	{
		switch (type)
		{
		case nim_chatroom::kAudio:
			is_audio_interact_ = !is_audio_interact_;
			break;
		case nim_chatroom::kVedio:
			is_vedio_interact_ = !is_vedio_interact_;
			break;
		default:
			break;
		}
	}

	void ChatroomForm::PopupCaptureChoiceMenu(POINT point)
	{
		CMenuWnd* pMenu = new CMenuWnd(NULL);
		STRINGorID xml(L"capture_choice_menu.xml");
		pMenu->Init(xml, _T("xml"), point);

		CMenuElementUI* single_button = (CMenuElementUI*)pMenu->FindControl(L"menu_singlewnd");
		CMenuElementUI* whole_button = (CMenuElementUI*)pMenu->FindControl(L"menu_wholewnd");
		CMenuElementUI* camera_button = (CMenuElementUI*)pMenu->FindControl(L"menu_camerawnd");

		single_button->AttachSelect(nbase::Bind(&ChatroomForm::OnSingleWndClick, this, std::placeholders::_1));
		whole_button->AttachSelect(nbase::Bind(&ChatroomForm::OnWholeWndClick, this, std::placeholders::_1));
		camera_button->AttachSelect(nbase::Bind(&ChatroomForm::OnCameraWndClick, this, std::placeholders::_1));

		pMenu->Show(this->GetHWND());
		pMenu->SetFocus(NULL);
	}

	bool ChatroomForm::OnSingleWndClick(ui::EventArgs* param)
	{
		nbase::ThreadManager::PostDelayedTask(kThreadUI, nbase::Bind(&ChatroomForm::ShowWindowSelect, this), nbase::TimeDelta::FromMilliseconds(200));
		return true;
	}

	bool ChatroomForm::OnWholeWndClick(ui::EventArgs* param)
	{
		btn_model_set_->SetText(L"全屏桌面");
		is_Camera_ = false;
		if (screen_tool_wnd_)
		{
			cur_capute_ = CaptureType::kCaptureTypeScreenAndCamera;
			cur_handle_id_ = GetDesktopWindow();
			OnLiveStreamInit(is_Camera_);
			screen_tool_wnd_->SetCaptureWnd(cur_handle_id_, false);
		}
		return true;
	}

	bool ChatroomForm::OnCameraWndClick(ui::EventArgs* param)
	{
		is_Camera_ = true;
		btn_model_set_->SetText(L"摄像头直播");
		if (screen_tool_wnd_)
		{
			cur_handle_id_ = 0;
			cur_capute_ = CaptureType::kCaptureTypeCamera;
			OnLiveStreamInit(is_Camera_);
		}
		return true;
	}

	void ChatroomForm::OnBtnEmoji()
	{
		if (!master_&&!is_pull_normal_)
			return;

		RECT rc = btn_face_->GetPos(true);
		POINT pt = { rc.left - 150, rc.top - 290 };
		::ClientToScreen(m_hWnd, &pt);

		OnSelectEmotion sel = nbase::Bind(&ChatroomForm::OnEmotionSelected, this, std::placeholders::_1);
		OnSelectEmotion2 sel2 = nbase::Bind(&ChatroomForm::OnEmotionSelectedSticker, this, std::placeholders::_1, std::placeholders::_2);
		OnEmotionClose  cls = nbase::Bind(&ChatroomForm::OnEmotionClosed, this);

		nim_comp::EmojiForm* emoji_form = new nim_comp::EmojiForm;
		emoji_form->ShowEmoj(pt, sel, sel2, cls, true);
	}

	bool ChatroomForm::OnSelChanged(ui::EventArgs* param)
	{
		EventType et = param->Type;
		std::wstring name = param->pSender->GetName();
		if (name == L"btn_face")
		{
			if (et == kEventSelect)
				OnBtnEmoji();
		}
		else if (name == L"option_intercommunicate")
			list_tab_->SelectItem(0);
		else if (name == L"option_online_members")
			list_tab_->SelectItem(1);
		else if (name == L"option_bypass_communicate")
			list_tab_->SelectItem(2);
		else if (name == L"option_bypass_gift")
			list_tab_->SelectItem(3);
		return true;
	}

	void ChatroomForm::OnEmotionSelected(std::wstring emo)
	{
		std::wstring file;
		if (nim_comp::emoji::GetEmojiFileByTag(emo, file))
		{
			nim_comp::emoji::InsertFaceToEdit(input_edit_, file, emo);
		}
	}

	void ChatroomForm::OnEmotionSelectedSticker(const std::wstring &catalog, const std::wstring &name)
	{

	}

	void ChatroomForm::OnEmotionClosed()
	{
		btn_face_->Selected(false, false);
		input_edit_->SetFocus();
	}

	bool ChatroomForm::OnSelOnlineMembers(ui::EventArgs* param)
	{
		// 单击在线成员tab后重新获取在线成员， 一分钟内不刷新
		time_t	sub_time = time(NULL) - time_refresh_;
		if (sub_time <= 10)
			return true;
		online_members_list_->RemoveAll();
		has_add_creater_ = false;
		managers_list_.clear();
		UpdateOnlineCount();
		GetMembers();
		return true;
	}

	bool ChatroomForm::OnSelBypassMembers(ui::EventArgs* param)
	{
		list_tab_->SelectItem(2);
		ShowBypassPrompt();
		return true;
	}

	bool ChatroomForm::OnLinkClick(WPARAM wParam, LPARAM lParam)
	{
		if (wParam == EN_LINK)
		{

		}
		return false;
	}

	bool ChatroomForm::OnMemberBypassMenu(ui::EventArgs* args)
	{
		Box* member_item = static_cast<Box*>(args->pSender);
		if (member_item != NULL)
		{
			ui::Label* name = (ui::Label*)member_item->FindSubControl(L"name");
			std::wstring uid = member_item->GetName();
			if (name != NULL)
			{
				ShowBypassOperateMenu(uid);
			}
		}
		return true;
	}

	void ChatroomForm::ShowBypassPrompt()
	{
		if (master_&&bypass_prompt_ != NULL)
		{
			if (option_bypass_members_->IsSelected())
				bypass_prompt_->SetVisible(false);
			else
				bypass_prompt_->SetVisible(true);
		}
	}


	void ChatroomForm::ShowBypassOperateMenu(std::wstring &uid)
	{
		if (uid.empty())
			return;

		//主播拥有操作该区域的权限
		if (creater_id_ == LoginManager::GetInstance()->GetAccount())
		{
			bypass_inact_account_.clear();
			bypass_inact_account_ = nbase::UTF16ToUTF8(uid);

			StdClosure cb = [this](){
				CMenuWnd* pMenu = new CMenuWnd(NULL);
				STRINGorID xml(L"bypass_menu.xml");
				POINT point;
				GetCursorPos(&point);
				pMenu->Init(xml, _T("xml"), point, CMenuWnd::RIGHT_TOP);
				//注册回调
				//这里根据状态还要进行判断
				CMenuElementUI* agree = (CMenuElementUI*)pMenu->FindControl(L"agree");
				agree->AttachSelect(nbase::Bind(&ChatroomForm::BypassAgreeMenuItemClick, this, std::placeholders::_1));

				CMenuElementUI* reject = (CMenuElementUI*)pMenu->FindControl(L"reject");
				reject->AttachSelect(nbase::Bind(&ChatroomForm::BypassRejectMenuItemClick, this, std::placeholders::_1));
				Label* reject_label = (Label*)pMenu->FindControl(L"reject_info");
				auto bypassmember = bypassmembers_list_.find(bypass_inact_account_);
				if (bypassmember != bypassmembers_list_.end())
				{
					switch (bypassmember->second.state)
					{
					case NTESLiveMicStateWaiting:
						agree->SetVisible(true);
						reject->SetVisible(false); //交互中未有拒绝这个选项，先屏蔽掉
						reject_label->SetText(L"拒绝");
						break;
					case NTESLiveMicStateConnecting:
						agree->SetVisible(false);
						reject->SetVisible(true);
						reject_label->SetText(L"取消连接");
						break;
					case NTESLiveMicStateConnected:
						agree->SetVisible(false);
						reject->SetVisible(true);
						reject_label->SetText(L"断开连接");
						break;
					default:
						break;
					}
					bypass_inact_type_ = bypassmember->second.type;
				}

				pMenu->Show(this->GetHWND());
			};
			Post2UI(cb);
		}
	}

	bool ChatroomForm::BypassAgreeMenuItemClick(ui::EventArgs* args)
	{
		if (!livestreaming_)
		{
			ShowMsgBox(m_hWnd, L"直播还未开启！", nullptr, L"提示", L"确定", L"");
			return true;
		}
		//主播同意连麦，发送自定义通知给用户id
		//如果已有连麦者，应该断开已有的连麦
		//更新连麦者状态

		if ((!bypassingaccout_.empty()) && bypassingaccout_ != bypass_inact_account_)
		{
			RequestPopMicLink(nbase::Int64ToString(room_id_), bypassingaccout_);
			SendSysNotifyMsg(kDisConnectingMIC, bypassingaccout_);
		}
		QLOG_APP(L"BypassAgreeMenuItemClick-bypass_inact_account:{0}") << bypass_inact_account_;
		SendSysNotifyMsg(kConnectingMIC, bypass_inact_account_);
		bypassingaccout_ = bypass_inact_account_;
		string nick = nbase::UTF16ToUTF8(nim_ui::UserManager::GetInstance()->GetUserNameW(bypass_inact_account_));
		UpdateBypassMembersState(bypass_inact_account_, NTESLiveMicStateConnecting, bypass_inact_type_, kUpdate, nick);
		return true;
	}

	bool ChatroomForm::BypassRejectMenuItemClick(ui::EventArgs* args)
	{
		//主播拒绝连麦或者主播主动断开连麦，发送自定义消息
		//1.pop 2.sendnotify
		if ((!bypassingaccout_.empty()) && bypassingaccout_ == bypass_inact_account_)
		{
			bypassingaccout_.clear();
		}
		RequestPopMicLink(nbase::IntToString(room_id_), bypass_inact_account_);
		SendSysNotifyMsg(kDisConnectingMIC, bypass_inact_account_);
		
		return true;
	}

	bool ChatroomForm::VChatMenuItemClick(ui::EventArgs* args)
	{
		if (clicked_user_account_.empty())
			return true;
		//nim_comp::VideoManager::GetInstance()->ShowVideoChatForm(clicked_user_account_, true);
		return true;
	}

	void ChatroomForm::SetMemberAdmin(const std::string &id, bool is_admin)
	{
		if (id == creater_id_)
			return;

		if (is_admin)
		{
			auto info = members_list_.find(id);
			if (info != members_list_.end())
			{
				managers_list_[id] = info->second;
			}

			ui::ButtonBox* member_item = (ui::ButtonBox*)online_members_list_->FindSubControl(nbase::UTF8ToUTF16(id));
			if (member_item)
			{
				ui::Control* member_type = (ui::Control*)member_item->FindSubControl(L"member_type");
				member_type->SetBkImage(L"icon_manager.png");
				online_members_list_->SetItemIndex(member_item, 1);
			}
		}
		else
		{
			auto manager_info = managers_list_.find(id);
			if (manager_info != managers_list_.end())
			{
				managers_list_.erase(manager_info);
			}

			ui::ButtonBox* member_item = (ui::ButtonBox*)online_members_list_->FindSubControl(nbase::UTF8ToUTF16(id));
			if (member_item)
			{
				ui::Control* member_type = (ui::Control*)member_item->FindSubControl(L"member_type");
				member_type->SetBkImage(L"");
				int manager_cout = managers_list_.size();
				if (has_add_creater_)
					manager_cout++;
				online_members_list_->SetItemIndex(member_item, manager_cout);
			}
		}
	}

	void ChatroomForm::SetMemberBlacklist(const std::string &id, bool is_black)
	{
		if (id == creater_id_)
			return;

		auto info = members_list_.find(id);
		if (info != members_list_.end())
		{
			info->second.is_blacklist_ = is_black;
			members_list_[id] = info->second;
		}
	}

	void ChatroomForm::SetMemberMute(const std::string &id, bool is_mute)
	{
		if (id == creater_id_)
			return;

		auto info = members_list_.find(id);
		if (info != members_list_.end())
		{
			info->second.is_muted_ = is_mute;
			members_list_[id] = info->second;
		}
	}

	void ChatroomForm::SetMemberFixed(const std::string &id, bool is_fixed)
	{
		if (id == creater_id_)
			return;

		auto info = members_list_.find(id);
		if (info != members_list_.end())
		{
			info->second.guest_flag_ = is_fixed ? kNIMChatRoomGuestFlagNoGuest : kNIMChatRoomGuestFlagGuest;
			members_list_[id] = info->second;
		}
	}

	void ChatroomForm::RemoveMember(const std::string &uid)
	{
		// 现在单击了在线成员列表后会重新刷新成员，无须单独维护成员列表
		auto exit_member = members_list_.find(uid);
		if (exit_member != members_list_.end())
		{
			if (!exit_member->second.is_blacklist_ && !exit_member->second.is_muted_ && exit_member->second.type_ == 0)
			{
				Control* member_item = online_members_list_->FindSubControl(nbase::UTF8ToUTF16(uid));
				online_members_list_->Remove(member_item);
				members_list_.erase(exit_member);
			}
		}
	}

	void ChatroomForm::UpdateOnlineCount()
	{
		ChatRoom::GetInfoAsync(room_id_, nbase::Bind(&ChatroomForm::OnGetChatRoomInfoCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	}

	void ChatroomForm::OnWndSizeMax(bool max)
	{
		if (!m_pRoot)
			return;

		FindControl(L"maxbtn")->SetVisible(!max);
		FindControl(L"restorebtn")->SetVisible(max);
	}

	void ChatroomForm::OnUserInfoChange(const std::list<nim::UserNameCard> &uinfos)
	{
		//for (auto iter = uinfos.cbegin(); iter != uinfos.cend(); iter++)
		//{
		//if (nim_ui::LoginManager::GetInstance()->IsEqual(iter->GetAccId()))
		//	InitHeader();
		//if (creater_id_ == iter->GetAccId())
		//	host_icon_->SetBkImage(nim_ui::PhotoManager::GetInstance()->GetUserPhoto(iter->GetAccId()));
		//return;
		//聊天室不建议对成员列表做监听后的实现，会有性能问题
		//ui::ButtonBox* room_member_item = (ui::ButtonBox*)online_members_list_->FindSubControl(nbase::UTF8ToUTF16(iter->GetAccId()));
		//if (room_member_item != NULL)
		//{
		//	ui::Control* header_image = (ui::Control*)room_member_item->FindSubControl(L"header_image");
		//	header_image->SetBkImage(nim_ui::PhotoManager::GetInstance()->GetUserPhoto(iter->GetAccId()));
		//}
		//}
	}

	void ChatroomForm::OnUserPhotoReady(PhotoType type, const std::string& account, const std::wstring& photo_path)
	{
		//if (type == kUser)
		//{
		//	if (nim_ui::LoginManager::GetInstance()->GetAccount() == account)
		//		header_icon_->SetBkImage(photo_path);
		//	if (creater_id_ == account)
		//		host_icon_->SetBkImage(photo_path);
		//}
		//return;
		//聊天室不建议对成员列表做监听后的实现，可能会有性能问题
		//ui::ButtonBox* room_member_item = (ui::ButtonBox*)online_members_list_->FindSubControl(nbase::UTF8ToUTF16(account));
		//if (room_member_item != NULL)
		//{
		//	ui::Control* header_image = (ui::Control*)room_member_item->FindSubControl(L"header_image");
		//	header_image->SetBkImage(photo_path);
		//}
	}

	void ChatroomForm::OnHttoDownloadReady(HttpResourceType type, const std::string& account, const std::wstring& photo_path)
	{
		if (type == kChatroomMemberIcon)
		{
			ui::ButtonBox* room_member_item = (ui::ButtonBox*)online_members_list_->FindSubControl(nbase::UTF8ToUTF16(account));
			if (room_member_item != NULL)
			{
				ui::Control* header_image = (ui::Control*)room_member_item->FindSubControl(L"header_image");
				header_image->SetBkImage(photo_path);
			}

			//if (nim_ui::LoginManager::GetInstance()->IsEqual(account))
			//	header_icon_->SetBkImage(photo_path);

			//if (account == creater_id_)
			//	host_icon_->SetBkImage(photo_path);
		}
	}

	//void ChatroomForm::InitHeader()
	//{
	//	std::string my_id = nim_ui::LoginManager::GetInstance()->GetAccount();
	//	nim_ui::UserManager* user_service = nim_ui::UserManager::GetInstance();
	//	header_icon_->SetBkImage(user_service->GetUserPhoto(my_id));
	//	name_->SetText(user_service->GetUserName(my_id, false));
	//}

	void ChatroomForm::RequestRoomError()
	{
		MsgboxCallback cb = [this](MsgBoxRet ret){
			Close();
			BypassLiveFontPage* frontPage = (BypassLiveFontPage*)nim_ui::WindowsManager::GetInstance()->GetWindow(nim_chatroom::BypassLiveFontPage::kClassName, nim_chatroom::BypassLiveFontPage::kClassName);
			frontPage->WindowReset();
		};

		ShowMsgBox(m_hWnd, L"进入房间失败！", ToWeakCallback(cb), L"提示", L"确定", L"");
	}

	void ChatroomForm::UpdateBypassMembersState(string uid, NTESLiveMicState micstate, InactionType type, BypassMemberListOpt opt, string nick)
	{
		StdClosure closure = [=]()
		{
			BypassMemberInfo bypassmerInfo;
			map<std::string, BypassMemberInfo>::iterator bypassmember = bypassmembers_list_.begin();
			switch (opt)
			{
			case nim_chatroom::kAdd:
			{
				//如果bypassmer
				bypassmember = bypassmembers_list_.find(uid);
				if (bypassmember != bypassmembers_list_.end())
					return;
				ui::ButtonBox* bypass_member_item = (ui::ButtonBox*)ui::GlobalManager::CreateBoxWithCache(L"chatroom/bypass_member_item.xml");
				bypassmerInfo.nick = nick;
				if (bypass_member_item)
				{
					bypass_member_item->AttachMenu(nbase::Bind(&ChatroomForm::OnMemberBypassMenu, this, std::placeholders::_1));
					ui::Control* header_image = (ui::Control*)bypass_member_item->FindSubControl(L"header_image");
					ui::Label* bypassmembername = (ui::Label*)bypass_member_item->FindSubControl(L"name");
					ui::Control* bypass_type = (ui::Control*)bypass_member_item->FindSubControl(L"bypass_type");
					ui::Control* bypass_state = (ui::Control*)bypass_member_item->FindSubControl(L"bypass_state");

					bypassmembername->SetUTF8Text(nick);
					bypassmerInfo.type = type;
					if (type == kAudio)
					{
						if (uid == LoginManager::GetInstance()->GetAccount())
						{
							is_vedio_interact_ = false;
							is_audio_interact_ = true;
						}
						bypass_type->SetBkImage(L"audio_y.png");
					}
					else
					{
						if (uid == LoginManager::GetInstance()->GetAccount())
						{
							is_audio_interact_ = false;
							is_vedio_interact_ = true;
						}
						bypass_type->SetBkImage(L"video_y.png");
					}
					bypassmerInfo.state = micstate;
					switch (micstate)
					{
					case nim_chatroom::NTESLiveMicStateWaiting:

						bypass_state->SetBkImage(L"icon_wait.png");
						break;
					case nim_chatroom::NTESLiveMicStateConnecting:
						bypass_state->SetBkImage(L"icon_connecting.png");
						break;

					case nim_chatroom::NTESLiveMicStateConnected:
						bypass_state->SetBkImage(L"icon_connected.png");
						break;
					default:
						break;
					}
					bypassmembers_list_[uid] = bypassmerInfo;
					bypass_member_item->SetUTF8Name(uid);
					bypass_members_list_->Add(bypass_member_item);
					ShowBypassPrompt();
				}
			}
			break;
			case nim_chatroom::kUpdate:
			{
				bypassmember = bypassmembers_list_.find(uid);
				if (bypassmember != bypassmembers_list_.end())
				{
					ui::ButtonBox* bypass_member_item = (ui::ButtonBox*)bypass_members_list_->FindSubControl(nbase::UTF8ToUTF16(uid));
					ui::Control* header_image = (ui::Control*)bypass_member_item->FindSubControl(L"header_image");
					ui::Label* bypassmembername = (ui::Label*)bypass_member_item->FindSubControl(L"name");
					ui::Control* bypass_type = (ui::Control*)bypass_member_item->FindSubControl(L"bypass_type");
					ui::Control* bypass_state = (ui::Control*)bypass_member_item->FindSubControl(L"bypass_state");

					if (type == kAudio)
					{
						bypass_type->SetBkImage(L"audio_y.png");
					}
					else
					{
						bypass_type->SetBkImage(L"video_y.png");
					}
					bypassmember->second.state = micstate;
					switch (micstate)
					{
					case nim_chatroom::NTESLiveMicStateWaiting:

						bypass_state->SetBkImage(L"icon_wait.png");
						break;
					case nim_chatroom::NTESLiveMicStateConnecting:
						bypass_state->SetBkImage(L"icon_connecting.png");
						break;

					case nim_chatroom::NTESLiveMicStateConnected:
						bypass_state->SetBkImage(L"icon_connected.png");
						break;
					default:
						break;
					}
				}
				break;

			case nim_chatroom::kRemove:
			{
				bypassmember = bypassmembers_list_.find(uid);
				if (bypassmember != bypassmembers_list_.end())
				{
					Control* member_item = bypass_members_list_->FindSubControl(nbase::UTF8ToUTF16(uid));
					bypass_members_list_->Remove(member_item);
					bypassmembers_list_.erase(bypassmember);
					if (uid == LoginManager::GetInstance()->GetAccount())
					{
						is_vedio_interact_ = false;
						is_audio_interact_ = false;
					}
				}
			}
			break;
			}
			}
		};
		Post2UI(closure);
	}

	void ChatroomForm::DeleteBypassMembers()
	{
		map<std::string, BypassMemberInfo>::iterator bypassmember = bypassmembers_list_.begin();
		if (bypassmember != bypassmembers_list_.end())
		{
			Control* member_item = bypass_members_list_->FindSubControl(nbase::UTF8ToUTF16(bypassmember->first));
			bypass_members_list_->Remove(member_item);
			bypassmembers_list_.erase(bypassmember);
			is_vedio_interact_ = false;
			is_audio_interact_ = false;
		}
	}

	void ChatroomForm::AudienceAfterAnchorOpt(NTESAnchorOperate opt)
	{
		string uid = LoginManager::GetInstance()->GetAccount();
		switch (opt)
		{
		case nim_chatroom::NTESAnchorAgree:
			//主播同意连麦
			//进入多人会议
			//进入成功之后由主播再全局发送自定义消息
		{
			if (!is_audio_interact_&&!is_vedio_interact_)
			{
				SendSysNotifyMsg(kRejectConnecting, creater_id_);
				return;
			}

			if (is_audio_interact_)
			{
				nim_comp::VideoManager::GetInstance()->SetVoipMode(nim::kNIMVideoChatModeAudio);
			}
			if (is_vedio_interact_)
			{
				nim_comp::VideoManager::GetInstance()->SetVoipMode(nim::kNIMVideoChatModeVideo);
			}
			string 	nick = nbase::UTF16ToUTF8(nim_ui::UserManager::GetInstance()->GetUserName(uid, false));
			UpdateBypassMembersState(uid, NTESLiveMicStateConnecting, (InactionType)bypass_inact_type_, nim_chatroom::kUpdate, nick);
			OnLiveStreamInit(true);
			SwitchPullStreamAndInteract(false);
		}
		break;
		case nim_chatroom::kNTESAnchorRejectOrDisConnect:
			//主播拒绝连麦请求
			//从会议队列中撤回连麦请求
		{
			RequestPopMicLink(nbase::IntToString(room_id_), uid);
            SwitchPullStreamAndInteract(true);
		}
			break;
		default:
			break;
		}
	}
}