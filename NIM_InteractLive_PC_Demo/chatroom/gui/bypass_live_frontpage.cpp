#include "bypass_live_frontpage.h"
/*#include "personal_center.h"*/
#include "callback/chatroom_callback.h"
#include "shared/xml_util.h"
#include "shared/ui/ui_menu.h"
#include "base/util/string_number_conversions.h"
#include "about/about_form.h"
#include "util/user.h"

static nim::NetDetectCbInfo info_;

namespace nim_chatroom
{
	const LPTSTR BypassLiveFontPage::kClassName = L"BypassLiveFontPage";
	using namespace ui;
#define LOGOFF_MSG	101

	BypassLiveFontPage::BypassLiveFontPage()
	{
	}

	BypassLiveFontPage::~BypassLiveFontPage()
	{
	}

	std::wstring BypassLiveFontPage::GetSkinFolder()
	{
		return L"chatroom";
	}

	std::wstring BypassLiveFontPage::GetSkinFile()
	{
		return L"meeting_frontpage.xml";
	}

	ui::UILIB_RESOURCETYPE BypassLiveFontPage::GetResourceType() const
	{
		return ui::UILIB_RESOURCETYPE::UILIB_FILE;
	}

	std::wstring BypassLiveFontPage::GetZIPFileName() const
	{
		return L"meeting_frontpage.zip";
	}

	std::wstring BypassLiveFontPage::GetWindowClassName() const
	{
		return kClassName;
	}

	std::wstring BypassLiveFontPage::GetWindowId() const
	{
		return kClassName;
	}

	UINT BypassLiveFontPage::GetClassStyle() const
	{
		return (UI_CLASSSTYLE_FRAME | CS_DBLCLKS);
	}

	void BypassLiveFontPage::InitWindow()
	{
		m_pRoot->AttachBubbledEvent(ui::kEventClick, nbase::Bind(&BypassLiveFontPage::OnClicked, this, std::placeholders::_1));
		id_edit_ = (ui::RichEdit*)FindControl(L"edit_room_id");
		video_model_ = (ui::Option*)FindControl(L"video_model");
		livingmodel_ = kVedio;
		video_model_->AttachSelect(nbase::Bind(&BypassLiveFontPage::OnSelVedioModel, this, std::placeholders::_1));
		audio_model_ = (ui::Option*)FindControl(L"audio_model");
		audio_model_->AttachSelect(nbase::Bind(&BypassLiveFontPage::OnSelAudioModel, this, std::placeholders::_1));

		//net detect
		net_detect_type_ = (ui::Box*)FindControl(L"net_detect_type");
		net_detect_loading_ = FindControl(L"net_detect_loading");
		net_detect_step_ = (ui::TabBox*)FindControl(L"net_detect_step");
		net_detect_tip_ = (ui::Box*)FindControl(L"net_detect_tip");
		net_detect_info_ = (ui::ButtonBox*)FindControl(L"net_detect_info");
		net_detect_info_->AttachBubbledEvent(ui::kEventMouseHover, nbase::Bind(&BypassLiveFontPage::Notify, this, std::placeholders::_1));
		net_detect_info_->AttachBubbledEvent(ui::kEventMouseLeave, nbase::Bind(&BypassLiveFontPage::Notify, this, std::placeholders::_1));
		tip_t_0_ = (ui::Label*)FindControl(L"tip_t_0");
		tip_t_1_ = (ui::Label*)FindControl(L"tip_t_1");
		tip_t_2_ = (ui::Label*)FindControl(L"tip_t_2");
		tip_t_3_ = (ui::Label*)FindControl(L"tip_t_3");
		tip_t_4_ = (ui::Label*)FindControl(L"tip_t_4");
		btn_join_room_ = (ui::Button*)FindControl(L"join_room");
		btn_create_room_ = (ui::Button*)FindControl(L"create_room");
		if (info_.res_code_ == 200)
		{
			NetDetectCb(info_.res_code_, info_);
		} 
		else
		{
			NetDetect();
		}
	}

	bool BypassLiveFontPage::OnSelAudioModel(ui::EventArgs* param)
	{
		livingmodel_ = kAudio;
		return true;
	}

	bool BypassLiveFontPage::OnSelVedioModel(ui::EventArgs* param)
	{
		livingmodel_ = kVedio;
		return true;
	}

	LRESULT BypassLiveFontPage::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{

		return __super::HandleMessage(uMsg, wParam, lParam);
	}

	bool BypassLiveFontPage::Notify(ui::EventArgs * msg)
	{
		std::wstring name = msg->pSender->GetName();
		if (name == L"net_detect_info")
		{
			switch (msg->Type)
			{
			case ui::kEventMouseLeave:
				net_detect_tip_->SetVisible(false);
				return true;
			case ui::kEventMouseHover:
				net_detect_tip_->SetVisible();
				break;
			}
		}
		return false;
	}

	bool BypassLiveFontPage::OnClicked(ui::EventArgs * msg)
	{
		std::wstring name = msg->pSender->GetName();
		if (name == L"close_btn")
		{
			auto cb = [this](MsgBoxRet ret)
			{
				if (ret == MB_YES)
				{
					Close();
				}
			};
			ShowMsgBox(m_hWnd, L"确定退出程序？", nbase::Bind(cb, std::placeholders::_1), L"提示", L"确定", L"取消");
		}
		else if (name == L"join_room")
		{
			btn_join_room_->SetEnabled(false);
			std::wstring edit_txt = id_edit_->GetText();
			uint64_t id = 0;
			nbase::StringToUint64(edit_txt, &id);
			if (id == 0)
			{
				ShowMsgBox(m_hWnd, L"房间不存在！", nullptr, L"提示", L"确定", L"");
				btn_join_room_->SetEnabled(true);
			}
			else
			{
				JoinRoom(id);
			}
		}
		else if (name == L"create_room")
		{
			//Todo
			btn_create_room_->SetEnabled(false);
			CreateMyRoomInfo();
			
		}
		else if (name == L"menu_button")
		{
			RECT rect = msg->pSender->GetPos();
			CPoint point;
			point.x = rect.left - 15;
			point.y = rect.bottom + 10;
			ClientToScreen(m_hWnd, &point);
			PopupMainMenu(point);
		}
		else if (name == L"net_detect")
		{
			NetDetect();
		}
		return false;
	}

	LRESULT BypassLiveFontPage::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		nim_ui::LoginManager::GetInstance()->DoLogout(true, nim::kNIMLogoutChangeAccout);
		return 0;
	}


	void BypassLiveFontPage::JoinRoom(int64_t id)
	{
		if (id != 0)
		{
			ChatroomForm* chat_form = new ChatroomForm(id);
			chat_form->Create(NULL, ChatroomForm::kClassName, WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX, 0, false);
			chat_form->SetNotifyExt(false);
			chat_form->SetAudience(true);
			chat_form->RequestEnter(id);
		}
	}

	void BypassLiveFontPage::CreateMyRoomInfo()
	{
		string roomname = nim::Tool::GetUuid();
		auto http_cb = [this,roomname](bool ret, int response_code, const std::string& reply)
		{
			int64_t room_id = 0;
			string  creator = "";
			int64_t  status = 0;
			string   pushUrl = "";
			string   rtmpPullUrl = "";
	        
			do
			{
				if (!ret || response_code != nim::kNIMResSuccess)
				{
					QLOG_ERR(L"Invoke get my room info error. Error code: {0}.") << response_code;
					std::wstring info = L"获取房间信息错误";
					StdClosure closure = [=]()
					{
						ShowMsgBox(m_hWnd,info, nullptr, L"提示", L"确定", L"");
						btn_create_room_->SetEnabled(true);
					};
					Post2UI(closure);
					break;
				}

				Json::Value json_reply;
				Json::Reader reader;
				if (reader.parse(reply, json_reply) && json_reply.isObject())
				{
					int res = json_reply["res"].asInt();
					if (res != 200)
					{
						StdClosure closure = [=]()
						{
							std::wstring info = L"获取房间信息错误";
							ShowMsgBox(m_hWnd, info, nullptr, L"提示", L"确定", L"");
							btn_create_room_->SetEnabled(true);

						};
						Post2UI(closure);
						QLOG_ERR(L"Invoke get my room info error. Json rescode: {0}, {1}.") << res << reply;
						break;
					}
					Json::Value json_info = json_reply["msg"];
					Json::Value json_reply_msg;
					room_id = json_reply["msg"]["roomid"].asInt64();
					rtmpPullUrl = json_reply["msg"]["live"]["rtmpPullUrl"].asString();
					pushUrl = json_reply["msg"]["live"]["pushUrl"].asString();
					status = json_reply["msg"]["status"].asInt();
				}
			} while (0);
			StdClosure closure = [this,room_id,pushUrl,roomname]()
			{
				if (room_id != 0)
				{
					nim_chatroom::ChatroomForm* chat_form = new nim_chatroom::ChatroomForm(room_id);
					chat_form->Create(NULL, ChatroomForm::kClassName, WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX, 0, false);
					chat_form->SetLivingModel(livingmodel_);
					chat_form->SetNotifyExt(true);
					chat_form->CenterWindow();
					chat_form->ShowWindow();
					chat_form->SetRtmpUrl(pushUrl);
					chat_form->SetChatRoomRoomName(roomname);
					chat_form->RequestEnter(room_id);
				}
				else
				{

				}
			};
			Post2UI(closure);
		};
		std::string server_addr = "https://app.netease.im";
		std::string addr = "/api/chatroom/hostEntrance";//"https://apptest.netease.im/api/chatroom/hostEntrance"
		std::string api_addr = server_addr + addr;
		std::string new_api_addr = GetConfigValue(g_AppServerAddress);
		if (!new_api_addr.empty())
		{
			api_addr = new_api_addr + addr;
		}
		std::string app_key = GetConfigValueAppKey();

	
		Json::FastWriter fw;
		Json::Value value;
		std::string avType = "AUDIO";
		if (livingmodel_==kVedio)
			avType = "VIDEO";
		value["type"] = kUnLive;
		value["meetingName"] =roomname;
		std::string body;
		string ext = fw.write(value);
		body += "uid=" + nim_comp::LoginManager::GetInstance()->GetAccount()
			+= "&ext=" + ext + "&avType=" + avType;

		nim_http::HttpRequest request(api_addr, body.c_str(), body.size(), ToWeakCallback(http_cb));
		request.AddHeader("Content-Type", "application/x-www-form-urlencoded;charset=utf-8");
		request.AddHeader("appKey", app_key);
		//request.SetMethodAsPost();
		nim_http::PostRequest(request);
	}

	void BypassLiveFontPage::PopupMainMenu(POINT point)
	{
		//创建菜单窗口
		CMenuWnd* pMenu = new CMenuWnd(NULL);
		STRINGorID xml(L"main_menu2.xml");
		pMenu->Init(xml, _T("xml"), point);
		//注册回调
		EventCallback look_log_cb = [this](ui::EventArgs* param){
			std::wstring dir = nim_ui::UserConfig::GetInstance()->GetUserDataPath();
			std::wstring tip = nbase::StringPrintf(L"当前用户数据目录：%s", dir.c_str());
			ShowMsgBox(m_hWnd, tip, MsgboxCallback(), L"提示", L"知道了", L"");
			auto cb_open_dir = [dir](){
				HINSTANCE inst = ::ShellExecute(NULL, L"open", dir.c_str(), NULL, NULL, SW_SHOW);
				int ret = (int)inst;
				if (ret > 32)
				{
					QLOG_APP(L"open user data path: {0}") << dir.c_str();
				}
				else
				{
					QLOG_ERR(L"failed to open user data path: {0}") << dir.c_str();
				}
			};
			nbase::ThreadManager::PostTask(kThreadGlobalMisc, cb_open_dir);
			return false;
		};
		CMenuElementUI* look_log = (CMenuElementUI*)pMenu->FindControl(L"look_log");
		look_log->AttachSelect(look_log_cb);

		EventCallback about_cb = [this](ui::EventArgs* param){
			nim_ui::WindowsManager::SingletonShow<AboutForm>(AboutForm::kClassName);
			return false;
		};
		CMenuElementUI* about = (CMenuElementUI*)pMenu->FindControl(L"about");
		about->AttachSelect(about_cb);

		EventCallback logoff_cb = [this](ui::EventArgs* param){
			QCommand::Set(kCmdRestart, L"true");
			std::wstring wacc = nbase::UTF8ToUTF16(nim_ui::LoginManager::GetInstance()->GetAccount());
			QCommand::Set(kCmdAccount, wacc);
			nim_ui::LoginManager::GetInstance()->DoLogout(false, nim::kNIMLogoutChangeAccout);
			return true;
		};
		CMenuElementUI* logoff = (CMenuElementUI*)pMenu->FindControl(L"logoff");
		logoff->AttachSelect(logoff_cb);

		EventCallback quit_cb = [this](ui::EventArgs* param){
			nim_ui::LoginManager::GetInstance()->DoLogout(false);
			return true;
		};
		CMenuElementUI* quit = (CMenuElementUI*)pMenu->FindControl(L"quit");
		quit->AttachSelect(quit_cb);
		//显示
		pMenu->Show();
	}
	void BypassLiveFontPage::NetDetect()
	{
		tip_t_0_->SetText(L"网络检测中...");
		tip_t_1_->SetVisible(false);
		tip_t_2_->SetVisible(false);
		tip_t_3_->SetVisible(false);
		tip_t_4_->SetVisible(false);
		net_detect_step_->SelectItem(0);
		net_detect_type_->SetBkImage(L"WiFi-0.png");
		net_detect_loading_->SetVisible();
		nim::VChat::NetDetect(nbase::Bind(&BypassLiveFontPage::NetDetectCb, this, std::placeholders::_1, std::placeholders::_2));
	}
	void BypassLiveFontPage::NetDetectCb(int code, nim::NetDetectCbInfo info)
	{
		net_detect_loading_->SetVisible(false);
		if (code == 200)
		{
			float p_flag = info.loss_ *1.0 / 20 * 0.5 + info.rtt_avg_*1.0 / 1200 * 0.25 + info.rtt_mdev_*1.0 / 150 * 0.25;
			if (p_flag <= 0.2625)
			{
				net_detect_step_->SelectItem(1);
				net_detect_type_->SetBkImage(L"WiFi-4.png");
			}
			else if (p_flag <= 0.55)
			{
				net_detect_step_->SelectItem(2);
				net_detect_type_->SetBkImage(L"WiFi-3.png");
			}
			else if (p_flag <= 1)
			{
				net_detect_step_->SelectItem(3);
				net_detect_type_->SetBkImage(L"WiFi-2.png");
			}
			else
			{
				net_detect_step_->SelectItem(4);
				net_detect_type_->SetBkImage(L"WiFi-1.png");
			}
			tip_t_0_->SetText(nbase::StringPrintf(L"丢 包 率：%d%%", info.loss_));
			tip_t_1_->SetText(nbase::StringPrintf(L"平均时延：%dms", info.rtt_avg_));
			tip_t_2_->SetText(nbase::StringPrintf(L"最大时延：%dms", info.rtt_max_));
			tip_t_3_->SetText(nbase::StringPrintf(L"最小时延：%dms", info.rtt_min_));
			tip_t_4_->SetText(nbase::StringPrintf(L"网络抖动：%dms", info.rtt_mdev_));
			tip_t_1_->SetVisible();
			tip_t_2_->SetVisible();
			tip_t_3_->SetVisible();
			tip_t_4_->SetVisible();
		} 
		else
		{
			net_detect_step_->SelectItem(5);
			tip_t_0_->SetText(L"获取网络参数失败！");
			tip_t_1_->SetVisible(false);
			tip_t_2_->SetVisible(false);
			tip_t_3_->SetVisible(false);
			tip_t_4_->SetVisible(false);
		}
	}
	void BypassLiveFontPage::SetNetDetectInfo(nim::NetDetectCbInfo info)
	{
		info_ = info;
	}

}