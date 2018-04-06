#include "chatroom_form.h"
#include "shared/ui/ui_menu.h"
#include "audio_hook_form.h"
#include "player/player_manager.h"
//#include "gui/about/about_form.h"
#include "gui/video/window_select.h"
#include "util/user.h"
#include "nim_cpp_sysmsg.h"
#include <sys/timeb.h>

namespace nim_chatroom
{
	using namespace ui;
	using namespace nim_comp;
	//直播
	void ChatroomForm::PaintVideo()
	{
		if (btn_stop_ls_->IsVisible() && start_time_stamp_ > 0)
		{
			timeb time_now;
			ftime(&time_now); // 秒数
			int64_t timestamp = time_now.time * 1000 + time_now.millitm; // 毫秒
			int64_t time_count = (timestamp - start_time_stamp_) / 1000;
			if (time_count > time_count_)
			{
				time_count_ = time_count;
				std::wstring time_tip = nbase::StringPrintf(L"%02lld:%02lld:%02lld", time_count_ / 3600, time_count_ % 3600 / 60, time_count_ % 60);
				ls_time_->SetText(time_tip);
			}
		}
		if (!video_show_ctrl_->Refresh(this, master_))
		{

		}
		if (bypass_show_ctrl_->IsVisible())
		{
			float pic_percent = 0.25;
			float right_percent = 0.1;
			float bottom_percent = 0.1;
			int master_ctrl_w = video_show_ctrl_->GetWidth();
			int master_ctrl_h = video_show_ctrl_->GetHeight();
			int master_pic_w = master_ctrl_w;
			int master_pic_h = master_ctrl_h;
			float master_pic_percent = video_show_ctrl_->GetPicWHPercent();
			if (master_pic_percent > 0)
			{
				if (master_ctrl_h * master_pic_percent >= master_ctrl_w)
				{
					master_pic_h = master_pic_w / master_pic_percent;
				} 
				else
				{
					master_pic_w = master_pic_h * master_pic_percent;
				}
			}
			bypass_show_ctrl_->SetMaxWidth(master_pic_w * pic_percent);
			bypass_show_ctrl_->SetMaxHeight(master_pic_h * pic_percent);
			int right_pos = (master_ctrl_w - master_pic_w) / 2 + master_pic_w * right_percent;
			int bottom_pos = (master_ctrl_h - master_pic_h) / 2 + master_pic_h * bottom_percent;
			bypass_show_ctrl_->SetMargin(ui::UiRect(0, 0, right_pos, bottom_pos));
			if (bypass_show_ctrl_->Refresh(this, !master_))
			{
			}
		}
		
	}
	void ChatroomForm::SetRtmpUrl(std::string rtmp_url)
	{
		rtmp_url_ = rtmp_url;
	}
	void ChatroomForm::CheckMaster()
	{
		if (creater_id_ == nim_ui::LoginManager::GetInstance()->GetAccount())
		{
			master_ = true;
			video_show_ctrl_->SetVideoFrameMng(&video_frame_mng_);
			bypass_show_ctrl_->SetVideoFrameMng(&nim_comp::VideoManager::GetInstance()->video_frame_mng_);
		}
		else
		{
			master_ = false;
			video_show_ctrl_->SetVideoFrameMng(&nim_comp::VideoManager::GetInstance()->video_frame_mng_);
			bypass_show_ctrl_->SetVideoFrameMng(&video_frame_mng_);
			video_show_ctrl_->SetAccount(creater_id_);

			check_chatroom_status_timer_.Cancel();
			
// 			StdClosure check_task = nbase::Bind(&ChatroomForm::CheckChatroomStatus, this);
// 			nbase::ThreadManager::PostRepeatedTask(kThreadUI, check_chatroom_status_timer_.ToWeakCallback(check_task), nbase::TimeDelta::FromMilliseconds(1000));
		}
		RequestAddress(nbase::Int64ToString(room_id_), nim_comp::LoginManager::GetInstance()->GetAccount());
	}

	bool ChatroomForm::OnLiveStreamInit(bool camera)
	{
		if (!livestreaming_)
		{
			if (master_)
				btn_start_ls_->SetVisible(true);
			btn_stop_ls_->SetVisible(false);
			time_count_ = 0;
			start_time_stamp_ = 0;
			ls_time_->SetText(L"00:00:00");
		}
		{
			CaptureType type;
			if (camera && screen_tool_wnd_)
			{
				type = kCaptureTypeCamera;
			}
			else
			{
				type = kCaptureTypeScreen;
				//ShowWindowSelect();
			}
			screen_tool_wnd_->StartCapture(type, 0, 45);
			{
				std::string def_device;
				int no = 0;
				nim_comp::VideoManager::GetInstance()->GetDefaultDevicePath(no, def_device, nim::kNIMDeviceTypeAudioIn);
				nim_comp::VideoManager::GetInstance()->StartDevice(nim::kNIMDeviceTypeAudioIn, def_device, kDeviceSessionTypeChatRoom);

 				nim_comp::VideoManager::GetInstance()->GetDefaultDevicePath(no, def_device, nim::kNIMDeviceTypeAudioOutChat);
				nim_comp::VideoManager::GetInstance()->StartDevice(nim::kNIMDeviceTypeAudioOutChat, def_device, kDeviceSessionTypeChatRoom);
			
				if ((type == kCaptureTypeCamera || type == kCaptureTypeScreenAndCamera) && (((!master_)&&is_vedio_interact_)||(master_&&living_model_==kVedio)))
				{
					nim_comp::VideoManager::GetInstance()->GetDefaultDevicePath(no, def_device, nim::kNIMDeviceTypeVideo);
					nim_comp::VideoManager::GetInstance()->StartDevice(nim::kNIMDeviceTypeVideo, def_device, kDeviceSessionTypeChatRoom);
				}
				else
				{
					nim_comp::VideoManager::GetInstance()->EndDevice(nim::kNIMDeviceTypeVideo, kDeviceSessionTypeChatRoom);
				}
			}
			ls_tab_->SelectItem(1);
			return true;
		}
		//ShowMsgBox(m_hWnd, L"设备初始化失败，请检查您的设备是否正常或者被占用", MsgboxCallback(), L"提示", L"确定", L"");
		return false;
	}
	void ChatroomForm::CreateRoomCallback(int code, __int64 channel_id, const std::string& json_extension)
	{
		QLOG_ERR(L"CreateRoomCallback code:{0}") << code;
		nim::VChat::Opt2Callback cb = nbase::Bind(&ChatroomForm::JoinRoomCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
		nim::NIMVideoChatMode mode = nim::kNIMVideoChatModeVideo;
		if (living_model_ == kAudio)
		{
			std::string def_device;
			int no = 0;
			nim_comp::VideoManager::GetInstance()->GetDefaultDevicePath(no, def_device, nim::kNIMDeviceTypeAudioIn);
			nim_comp::VideoManager::GetInstance()->StartDevice(nim::kNIMDeviceTypeAudioIn, def_device, kDeviceSessionTypeChatRoom);
			mode = nim::kNIMVideoChatModeAudio;
		}
		
		if (nim_comp::VideoManager::GetInstance()->JoinRoom(mode, GetRoomName(), GetRoomSessionId(), rtmp_url_, true, cb))
		{
			nim_comp::VideoManager::GetInstance()->SetChatRoomCb(nbase::Bind(&ChatroomForm::RoomConnectCallback, this, std::placeholders::_1),
				nbase::Bind(&ChatroomForm::RoomPeopleChangeCallback, this, std::placeholders::_1, std::placeholders::_2));
		}
		else
		{
			StartLiveStreamRet(false);
		}
	}
	void ChatroomForm::JoinRoomCallback(int code, __int64 channel_id, const std::string& json_extension)
	{
		QLOG_ERR(L"JoinRoomCallback code:{0}") << code;
		StartLiveStreamRet(code == nim::kNIMVChatConnectSuccess);
		if (code == nim::kNIMVChatConnectSuccess)
		{
			channel_id_ = channel_id;
			if (!master_)
			{
				//nim_comp::PlayerManager::GetInstance()->StopPlay();
			}
			else
			{
				SendInitMsg();
			}
		}
	}

	void ChatroomForm::RoomConnectCallback(int code)
	{
		bool successed = code == nim::kNIMVChatConnectSuccess;
		bool logout = code == nim::kNIMVChatConnectLogout;
		bool disconnect = code == nim::kNIMVChatChannelDisconnected;
		if (livestreaming_)
		{
			if (successed)
			{
				//主播kNIMVChatVideoQuality720p || kNIMVChatVideoQualitySuper
				//观众 kNIMVChatVideoQualityNormal
				if (master_)
				{
					nim::VChat::SetVideoQuality(nim::kNIMVChatVideoQuality720p);
					
				}
				else
				{
					nim::VChat::SetVideoQuality(nim::kNIMVChatVideoQualityNormal);
					StdClosure closue = [=]()
					{
						string uid = LoginManager::GetInstance()->GetAccount();
						string nick = nbase::UTF16ToUTF8(nim_ui::UserManager::GetInstance()->GetUserNameW(uid));
						UpdateBypassMembersState(uid, NTESLiveMicStateConnected, (InactionType)bypass_inact_type_, nim_chatroom::kAdd, nick);
						if (is_vedio_interact_)
						{
							bypass_show_ctrl_->SetVisible(true);
							bypass_show_ctrl_->SetAccount(uid);
						}
						else
						{
							bypass_show_ctrl_->SetVisible(false);
							bypass_show_ctrl_->Clear();
						}

					};
					Post2UI(closue);
				}
			}
			else
			{
				bypass_show_ctrl_->SetVisible(false);
				OnLsErrorCallback(code);
			}
		}
		else if (logout)
		{
			if (interact_stopped_cb_.get() != NULL)
				(*interact_stopped_cb_.get())();
		}
		else
		{
			StartLiveStreamRet(successed);
		}
	}

	void ChatroomForm::RoomPeopleChangeCallback(std::string uid, bool join_type)
	{
		QLOG_APP(L"RoomPeopleChangeCallback-uid:{0},join_type:{1}") << uid << join_type;
		//pre_uid_ = "";
		string account=nim_ui::LoginManager::GetInstance()->GetAccount();

		if (((pre_uid_ !=uid&&uid!=creater_id_)|| pre_uid_ == account)&&join_type)
		{
			if (bypass_inact_type_ == kAudio)
				bypass_show_ctrl_->SetVisible(false);
			else
			{
				bypass_show_ctrl_->SetAccount(uid);
				bypass_show_ctrl_->SetVisible(true);
			}
			pre_uid_ = uid;
		}
		else if (pre_uid_ == uid && !join_type)
		{
			bypass_show_ctrl_->SetVisible(false);
			pre_uid_ = "";
		}
		//发送一条自定义消息
		if (join_type)
		{
			if (master_)
			{
				SendCustomMsg(NTESAnchorAgree, uid, bypass_inact_type_);
			}
			//更改连接状态
			if (uid != creater_id_)
			{				
				string nick = nbase::UTF16ToUTF8(nim_ui::UserManager::GetInstance()->GetUserNameW(uid, false));
				QLOG_APP(L"RoomPeopleChangeCallback-uid:{0},nick:{1}") << uid << nick;
				//更新状态
				UpdateBypassMembersState(uid, NTESLiveMicStateConnected, (InactionType)bypass_inact_type_, kUpdate, nick);
				UpdateChatRoomQueue(uid, NTESLiveMicStateConnected);
			}
			else
			{
				bypassingaccout_ = uid;
			}
		}
		else
		{
			//主播退出
			//更新UI状态
			//
			if (uid == creater_id_)
			{
				
				StdClosure closure = [this]()
				{
					if (livestreaming_)
						StopLiveStream();
				    DeleteBypassMembers();
					auto cb = [this](MsgBoxRet)
					{
						Close();
					};
					QLOG_ERR(L"RoomPeopleChangeCallback:the living end");
					ShowMsgBox(m_hWnd,L"直播已结束",cb, L"提示", L"确定", L"");

				};

				Post2UI(closure);
				return;
			}

			if (master_)
			{
				SendCustomMsg(kNTESAnchorRejectOrDisConnect, uid, bypass_inact_type_);
				ChatRoom::QueuePollAsync(room_id_, uid,nullptr);
			}
			string nick = nbase::UTF16ToUTF8(nim_ui::UserManager::GetInstance()->GetUserNameW(uid, false));
			UpdateBypassMembersState(uid, NTESLiveMicStateConnected, (InactionType)bypass_inact_type_, kRemove, nick);
		}

		if (master_)
		{
			bypass_show_ctrl_->Clear();
		} 
		else
		{
			video_show_ctrl_->Clear();
		}

	}

	void ChatroomForm::PushVideoStream()
	{
		if (livestreaming_)
		{
			int width = kScreenDefWidth;
			int height = kScreenDefHeight;
			int size = width * height * 3 / 2;
			std::string data;
			data.resize(size);
			int64_t time = 0;
			bool ret = video_frame_mng_.GetVideoFrame("", time, (char*)data.c_str(), width, height, false, false);
			if (ret)
			{
				size = width * height * 3 / 2;
				nim::VChat::CustomVideoData(time, data.c_str(), size, width, height, "");
			}
		}
	}

	bool ChatroomForm::StartLiveStream()
	{
		bool ret = false;
		if (!livestreaming_)
		{
			if (master_)

			{
				nim::VChat::Opt2Callback cb = nbase::Bind(&ChatroomForm::CreateRoomCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
				nim_comp::VideoManager::GetInstance()->CreateRoom(GetRoomName(), "", cb);
				ret = true;
			}
			else
			{
				nim::NIMVideoChatMode mode = nim::kNIMVideoChatModeVideo;
				if (((!master_) && is_audio_interact_) || living_model_ == kAudio)
					mode = nim::kNIMVideoChatModeAudio;
			
				nim::VChat::Opt2Callback cb = nbase::Bind(&ChatroomForm::JoinRoomCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
				if (nim_comp::VideoManager::GetInstance()->JoinRoom(mode, GetRoomName(), GetRoomSessionId(), "", true, cb))
				{
					nim_comp::VideoManager::GetInstance()->SetChatRoomCb(nbase::Bind(&ChatroomForm::RoomConnectCallback, this, std::placeholders::_1),
						nbase::Bind(&ChatroomForm::RoomPeopleChangeCallback, this, std::placeholders::_1, std::placeholders::_2));
					ret = true;
				}
			}
		}
		return ret;
	}

	void ChatroomForm::StartLiveStreamRet(bool ret)
	{
		livestreaming_ = ret;
		if (ret)
		{
			btn_start_ls_->SetVisible(false);
			if (master_)
				btn_stop_ls_->SetVisible(true);
			time_count_ = 0;
			timeb time_now;
			ftime(&time_now); // 秒数
			start_time_stamp_ = time_now.time * 1000 + time_now.millitm; // 毫秒
			ls_time_->SetText(L"00:00:00");
			push_video_timer_.Cancel();

			//更新聊天室信息
			UpdateChatroomInfo(true);
			StdClosure task = nbase::Bind(&ChatroomForm::PushVideoStream, this);
			nbase::ThreadManager::PostRepeatedTask(kThreadLiveStreaming, push_video_timer_.ToWeakCallback(task), nbase::TimeDelta::FromMilliseconds(50));
		}
		else
		{
			push_video_timer_.Cancel();
			if (master_)
				btn_start_ls_->SetVisible(true);
			btn_stop_ls_->SetVisible(false);
			ShowMsgBox(m_hWnd, L"开启直播失败", MsgboxCallback(), L"提示", L"确定", L"");
		}
	}

	bool ChatroomForm::OnStartLiveStream(ui::EventArgs* msg)
	{
		if (is_Camera_)
			OnLiveStreamInit(true);
		else if (screen_tool_wnd_)
		{
			OnLiveStreamInit(false);
			screen_tool_wnd_->SetCaptureWnd(cur_handle_id_, false);
		}
		
		StartLiveStream();
		//{
			//ShowMsgBox(m_hWnd, L"开启直播失败", MsgboxCallback(), L"提示", L"确定", L"");
		//}
		//else
		{
			btn_start_ls_->SetVisible(false);
			if (master_)
				btn_stop_ls_->SetVisible(true);
			time_count_ = 0;
			start_time_stamp_ = 0;
			ls_time_->SetText(L"00:00:00");
		}
		return true;
	}
	bool ChatroomForm::OnStopLiveStream(ui::EventArgs* msg)
	{
		MsgboxCallback cb = [this](MsgBoxRet ret){
			if (ret == MB_YES)
			{
				UpdateChatroomInfo(false);
				StopLiveStream();
			}
		};

		ShowMsgBox(m_hWnd, L"确定结束直播？", ToWeakCallback(cb), L"提示", L"结束", L"取消");
		return true;
	}

	void ChatroomForm::StopLiveStream()
	{
		if (screen_tool_wnd_ != nullptr)
		{
			screen_tool_wnd_->StopCapture();
		}
		bypass_show_ctrl_->SetVisible(false);
		bypass_show_ctrl_->Clear();
		livestreaming_ = false;
		push_video_timer_.Cancel();
		pre_uid_ = "";
		nim_comp::VideoManager::GetInstance()->EndChat(GetRoomSessionId());
		if (master_)
			btn_start_ls_->SetVisible(true);
		
		btn_stop_ls_->SetVisible(false);
		time_count_ = 0;
		start_time_stamp_ = 0;
		is_audio_interact_ = false;
		is_vedio_interact_ = false;
	
	}

	void ChatroomForm::OnLsErrorCallback(int errCode)
	{
		auto cb = [this](int errCode)
		{
			StopLiveStream();
			std::wstring tip = nbase::StringPrintf(L"直播出错 (code:%d)", errCode);
			ShowMsgBox(m_hWnd, tip, nullptr, L"提示", L"确定", L"");
		};

		nbase::ThreadManager::PostTask(kThreadUI, ToWeakCallback(nbase::Bind(cb, errCode)));
	}

	void ChatroomForm::ChangeCamera(bool open)
	{
		if (open&&living_model_==kVedio)
		{
			std::string def_device;
			int no = 0;
			VideoManager::GetInstance()->GetDefaultDevicePath(no, def_device, nim::kNIMDeviceTypeVideo);
			VideoManager::GetInstance()->StartDevice(nim::kNIMDeviceTypeVideo, def_device, kDeviceSessionTypeChatRoom);
		}
		else
		{
			VideoManager::GetInstance()->EndDevice(nim::kNIMDeviceTypeVideo, kDeviceSessionTypeChatRoom);
		}
	}

	void ChatroomForm::CloseDriver()
	{
		VideoManager::GetInstance()->EndDevice(nim::kNIMDeviceTypeAudioIn, kDeviceSessionTypeChatRoom);
		VideoManager::GetInstance()->EndDevice(nim::kNIMDeviceTypeVideo, kDeviceSessionTypeChatRoom);
		VideoManager::GetInstance()->EndDevice(nim::kNIMDeviceTypeAudioOut, kDeviceSessionTypeChatRoom);
		nim::VChat::EndDevice(nim::kNIMDeviceTypeAudioHook);
		AudioHookForm::ResetAudioHookType();
		AudioHookForm* audio_hook = (AudioHookForm*)nim_ui::WindowsManager::GetInstance()->GetWindow(AudioHookForm::kClassName, AudioHookForm::kClassName);
		if (audio_hook != NULL)
		{
			audio_hook->Close();
		}
	}

	void ChatroomForm::ShowWindowSelect()
	{
		SelectWndCallback cb = nbase::Bind(&ChatroomForm::SelectWndCb, this, std::placeholders::_1, std::placeholders::_2);
		WindowSelectForm *window = (WindowSelectForm*)(nim_comp::WindowsManager::GetInstance()->GetWindow(WindowSelectForm::kClassName, WindowSelectForm::kClassName));
		if (window)
		{
			window->ShowWindow(true, true);
			window->SetSelWndCb(cb);
			SetForegroundWindow(window->GetHWND());
			return;
		}
		else
		{
			window = new WindowSelectForm();
			window->Create(NULL, L"选择窗口", WS_OVERLAPPEDWINDOW, 0);
			window->CenterWindow();
			window->ShowWindow();
			window->SetSelWndCb(cb);
		}
	}
	
	void ChatroomForm::SelectWndCb(HWND id, bool cut)
	{
		if (id != 0 && screen_tool_wnd_)
		{
			is_Camera_ = false;
			OnLiveStreamInit(is_Camera_);
			screen_tool_wnd_->SetCaptureWnd(id, cut);
		}
	}

	void ChatroomForm::RequestPushMicLink(std::string roomid, std::string uid, std::string ext,InactionType type)
	{
		auto http_cb = [this,uid,type](bool ret, int response_code, const std::string& reply)
		{
			if (ret && response_code == 200) {
				Json::Value json;
				Json::Reader reader;
				bool res = reader.parse(reply, json);
				if (!res)
				{
					return;
				}
				int json_res = json["res"].asInt();
				if (json_res == 200)
				{
					QLOG_ERR(L"RequestPushMicLink sucess. Error code: {0}.") << json_res;
					//加入队列成功
					//发送自定义系统通知给主播
					
					if (type == kAudio)
					{
						is_audio_interact_ = true;
						is_vedio_interact_ = false;
					}
					else
					{
						is_audio_interact_ = false;
						is_vedio_interact_ = true;
					}

					this->SendSysNotifyMsg(kJoinQueue,creater_id_);
				}
				else
				{
					StdClosure closure = [this,json_res]()
					{
						MsgboxCallback cb = [](MsgBoxRet ret){
							//this->Close();
						};
						std::wstring info = L"进入队列失败.Error Code:{0}" + nbase::IntToString16(json_res);
						ShowMsgBox(m_hWnd, info, ToWeakCallback(cb), L"提示", L"确定", L"");
						is_vedio_interact_ = false;
						is_vedio_interact_ = false;
					};
					Post2UI(closure);
					QLOG_ERR(L"RequestPushMicLink error. Error code: {0}.") << json_res;
				}
			}
			else {
				QLOG_ERR(L"RequestPushMicLink error. Error code: {0}.") << response_code;
				is_audio_interact_ = false;
				is_vedio_interact_ = false;
			}
		};

		std::string server_addr = "https://app.netease.im";
		std::string addr = "/api/chatroom/pushMicLink";//""https://apptest.netease.im/api/chatroom/pushMicLink""
		std::string api_addr = server_addr + addr;
		std::string new_api_addr = GetConfigValue(g_AppServerAddress);
		if (!new_api_addr.empty())
		{
			api_addr = new_api_addr + addr;
		}
		std::string app_key = GetConfigValueAppKey();
		std::string body;
		body += "roomid="+roomid+
			    "&uid="+uid+
			    "&ext="+ext;
		nim_http::HttpRequest request(api_addr, body.c_str(), body.size(), http_cb);
		request.AddHeader("Content-Type", "application/x-www-form-urlencoded;charset=utf-8");
		request.AddHeader("appKey", app_key);
		nim_http::PostRequest(request);
	}

	void ChatroomForm::RequestPopMicLink(std::string roomid, std::string uid)
	{
		auto http_cb = [this,uid](bool ret, int response_code, const std::string& reply)
		{
			if (ret && response_code == 200) {
				Json::Value json;
				Json::Reader reader;
				bool res = reader.parse(reply, json);
				if (!res)
				{
					return;
				}
				int json_res = json["res"].asInt();
				if (json_res == 200)
				{

					QLOG_ERR(L"RequestPushMicLink sucess. Error code: {0}.") << json_res;
					//成功
					auto action = [this,uid](){
						string 	nick = nbase::UTF16ToUTF8(nim_ui::UserManager::GetInstance()->GetUserName(uid, false));
						UpdateBypassMembersState(uid, NTESLiveMicStateConnected, bypass_inact_type_, kRemove,nick);
						is_audio_interact_ = false;
						is_vedio_interact_ = false;
						//给主播发送一条自定义消息
						this->SendSysNotifyMsg(kExitQueue, creater_id_);
						
					};
					Post2UI(action);
				}
				else
				{
					is_audio_interact_ = false;
					is_vedio_interact_ = false;
					QLOG_ERR(L"RequestPushMicLink error. Error code: {0}.") << json_res;
				}
			}
			else {
				QLOG_ERR(L"RequestPushMicLink error. Error code: {0}.") << response_code;
				is_audio_interact_ = false;
				is_vedio_interact_ = false;
			}
		};

		std::string server_addr = "https://app.netease.im";
		std::string addr = "/api/chatroom/popMicLink";//""https://apptest.netease.im/api/chatroom/popMicLink""
		std::string api_addr = server_addr + addr;
		std::string new_api_addr = GetConfigValue(g_AppServerAddress);
		if (!new_api_addr.empty())
		{
			api_addr = new_api_addr + addr;
		}

		std::string app_key = GetConfigValueAppKey();
		std::string body;
		body += "roomid=" + roomid +
			    "&uid=" + uid;

		nim_http::HttpRequest request(api_addr, body.c_str(), body.size(), http_cb);
		request.AddHeader("Content-Type", "application/x-www-form-urlencoded;charset=utf-8");
		request.AddHeader("appKey", app_key);
		nim_http::PostRequest(request);
	}

	void ChatroomForm::RequestAddress(string roomid, string uid)
	{
		auto http_cb = [=](bool ret, int response_code, const std::string& reply)
		{
			if (ret && response_code == 200) {
				Json::Value json;
				Json::Reader reader;
				std::wstring pullurl = L"拉流地址为：";
				bool res = reader.parse(reply, json);
				if (!res)
				{
					return;
				}
				int json_res = json["res"].asInt();
				if (json_res == 200)
				{
					rtmp_pull_url_ = nbase::UTF8ToUTF16(json["msg"]["live"]["rtmpPullUrl"].asString());
					pullurl.append(rtmp_pull_url_);
					//成功
					auto action = [this, pullurl]()
					{
						bulletin_->SetBkImage(L"");
						bulletin_->SetText(pullurl);
					};
					Post2UI(action);
				}
				else
				{
					RequestAddress(roomid, uid);
					QLOG_ERR(L"RequestAddress error. Error code: {0}.reply:{1}") << json_res << reply;
				}
			}
			else 
			{
				RequestAddress(roomid, uid);
				QLOG_ERR(L"RequestAddress error. Error code: {0}.reply:{1}") << response_code << reply;
			}
			//非主播进入之前查询一次状态
			if (!master_)
				CheckChatroomStatus();
		};
		std::string server_addr = "https://app.netease.im";
		std::string addr = "/api/chatroom/requestAddress";
		std::string api_addr = server_addr + addr;
		std::string new_api_addr = GetConfigValue(g_AppServerAddress);
		if (!new_api_addr.empty())
		{
			api_addr = new_api_addr + addr;
		}

		//std::string api_addr = "https://apptest.netease.im/api/chatroom/requestAddress";
		std::string app_key = GetConfigValueAppKey();
		Json::FastWriter fw;
		Json::Value value;
		value["roomid"] = roomid;
		value["uid"] = uid;
		string body = fw.write(value);

		nim_http::HttpRequest request(api_addr, body.c_str(), body.size(), http_cb);
		request.AddHeader("Content-Type", "application/json;charset=utf-8");
		request.AddHeader("appKey", app_key);
		nim_http::PostRequest(request);
	}




	void ChatroomForm::InitChatRoomQueueInfo()
	{
		if (master_)
		{
			auto queuedropcb = [this](__int64 room_id, int error_code)
			{
				if (error_code==200)
				{
					QLOG_ERR(L"QueueDropAsync sucess. Error code: {0}.") << error_code;
				}
				else
				{

				}
			};
			ChatRoom::QueueDropAsync(room_id_, queuedropcb);
		}
		else
		{
			auto queuelistCallback = [this](__int64 room_id, int error_code, const ChatRoomQueue& queue)
			{
				//更新ui信息
				StdClosure closure = [this, room_id, queue]{
					auto it = queue.begin();
					Json::Reader reader;
					Json::Value  value;
					for (; it != queue.end(); it++)
					{
						int state;
						int style;
						string nick;
						if (reader.parse(it->value_, value))
						{
							//{"info":"{\"avatar\":\"avatar_default\",\"nick\":\"003\"}", "state" : 2, "style" : 1}
							state = value["state"].asInt();
							style = value["style"].asInt();

							if (value["info"].isObject())
							{
								nick = value["info"]["nick"].asString();
							}
							else
							{
								string info = value["info"].asString();
								Json::Value info_value;
								if (reader.parse(info, info_value))
								{
									nick = info_value["nick"].asString();
								}
							}
							
						}

					   QLOG_ERR(L"QueueListAsync sucess. QueueList_Key: {0}.") << it->key_;
					
						//后进的观众进来后只显示已连麦的观众
					   if (it->key_ != creater_id_&&state == NTESLiveMicStateConnected)
							UpdateBypassMembersState(it->key_, (NTESLiveMicState)state, (InactionType)style, kAdd, nick);
					}
				};
				Post2UI(closure);
			};
			string ext;
			ChatRoom::QueueListAsync(room_id_, queuelistCallback, ext);
		}
	}

	void ChatroomForm::UpdateChatRoomQueue(string uid, NTESLiveMicState state)
	{
		//{"info":"{\"avatar\":\"avatar_default\",\"nick\":\"005\"}", "state" : 1, "style" : 2}
		auto it = bypassmembers_list_.find(uid);
		if (it == bypassmembers_list_.end())
			return;
		Json::FastWriter fw;
		string str_value;
		Json::Value json_value;

		ChatRoomQueueElement element;
		element.key_ = uid;

		Json::Value json_info;
		json_info["avatar"] = "avatar_default";
		json_info["nick"] = it->second.nick;
		string str_info = fw.write(json_info);
		json_value["info"] = str_info;
		json_value["state"] = state;
		json_value["style"] = it->second.type;

		element.value_ = fw.write(json_value);
		auto queueoffercb = [this](__int64 room_id, int error_code){
			if (error_code == 200)
			{
				QLOG_ERR(L"QueueOfferAsync sucess. Error code: {0}.") << error_code;
			}
			else
			{

			}
		};
		string json_ext;
		ChatRoom::QueueOfferAsync(room_id_, element, queueoffercb, json_ext);
	}

	void ChatroomForm::SetLivingModel(InactionType living_model)
	{
		living_model_ = living_model;
		switch (living_model_)
		{
		case nim_chatroom::kAudio:
			btn_camera_set_ ->SetVisible(false);
			btn_model_set_->SetVisible(false);
			break;
		case nim_chatroom::kVedio:
			btn_camera_set_->SetVisible(true);
			btn_model_set_->SetVisible(true);
			break;
		default:
			break;
		}
	}

	void ChatroomForm::PullPlayError(int code, const std::string& msg)
	{
		if (code == 200)
		{
			return;
		}
		else
			QLOG_ERR(L"Play error:code:{0};message:{1}")<<code<<msg;
		
		
		StdClosure closure = [this]()
		{
			auto cb = [this](MsgBoxRet)
			{
				ChatRoom::Exit(room_id_);
				Close();
			};
			ShowMsgBox(m_hWnd, L"观看直播失败，请重进!", cb, L"提示", L"确定", L"");

		};
		Post2UI(closure);
	}

	void ChatroomForm::CheckChatroomStatus()
	{
		auto cb = [this](__int64 room_id, int error_code, const ChatRoomInfo& info)
		{
			Json::Value value;
			Json::Reader reader;
			QLOG_APP(L"bypassdemo-CheckChatroomStatus:info.ext-{0}") << info.ext_;
			bool res=reader.parse(info.ext_, value);
			std::wstring prompt_info = L"";
			if (res&&value.isObject())
			{
				int statu = value["type"].asInt();
				if (statu == kUnLive)
				{
					prompt_info = L"直播已结束";
					StdClosure closure = [this, prompt_info]()
					{
						auto box_cb = [this](MsgBoxRet)
						{
							//ChatRoom::Exit(room_id_);
							Close();
						};
						check_chatroom_status_timer_.Cancel();
						ShowMsgBox(m_hWnd, prompt_info, box_cb, L"提示", L"确定", L"");
					};
					Post2UI(closure);
				}
				else
				{
					StdClosure closure = [this]()
					{
						nim_comp::PlayerManager* player_manager = nim_comp::PlayerManager::GetInstance();
						unregister_cb.Add(player_manager->RegPlayerStateCallback(nbase::Bind(&ChatroomForm::OnPullStreamPlayerStateChange, this, std::placeholders::_1)));
						SwitchPullStreamAndInteract(true);
					};
					Post2UI(closure);
					
				}
			}
		};

		ChatRoom::GetInfoAsync(room_id_, cb);
	}

	void ChatroomForm::UpdateChatroomInfo(bool live)
	{
		auto cb = [this, live](__int64 room_id, int error_code, const ChatRoomInfo& info)
		{
			//{"meetingName":"acbbbddebd40b1458a594f1428a21ac9","type":2}
			ChatRoomInfo temp =  info;
			if (error_code == 200)
			{
				Json::Value value;
				Json::Reader reader;
				Json::FastWriter fw;
				int res = reader.parse(temp.ext_, value);
				if (res)
				{
					if (live)
					{
						value["type"] = living_model_;
					}
					else
					{
						value["type"] = -1;
					}
				}
				temp.ext_ = fw.write(value);
				auto updata_cb = [this](__int64 room_id, int error_code)
				{
					QLOG_ERR(L"UpdateRoomInfoAsync. Error code: {0}.") << error_code;
				};
				
				ChatRoom::UpdateRoomInfoAsync(room_id_, temp, true, temp.ext_, updata_cb, "");
			}
		};
		ChatRoom::GetInfoAsync(room_id_, cb);
	}

	void ChatroomForm::UpdateChatroomInfoWithClose(bool live)
	{
		auto cb = [this, live](__int64 room_id, int error_code, const ChatRoomInfo& info)
		{
			//{"meetingName":"acbbbddebd40b1458a594f1428a21ac9","type":2}
			ChatRoomInfo temp = info;
			if (error_code == 200)
			{
				Json::Value value;
				Json::Reader reader;
				Json::FastWriter fw;
				int res = reader.parse(temp.ext_, value);
				if (res)
				{
					if (live)
					{
						value["type"] = living_model_;
					}
					else
					{
						value["type"] = kUnLive;
					}
				}
				temp.ext_ = fw.write(value);
				auto updata_cb = [this](__int64 room_id, int error_code)
				{
					QLOG_ERR(L"UpdateRoomInfoAsync. Error code: {0}.") << error_code;
					//ChatRoom::Exit(room_id_);
					StdClosure closure = [this]()
					{
						Close();
					};
					Post2UI(closure);
				};

				ChatRoom::UpdateRoomInfoAsync(room_id_, temp, true, temp.ext_, updata_cb, "");
			}
		};
		ChatRoom::GetInfoAsync(room_id_, cb);
	}

	void ChatroomForm::OnPullStreamPlayerStateChange(nim_comp::EN_PLAYER_STATE state)
	{
		StdClosure closure = [=]()
		{
			switch (state)
			{
			case nim_comp::EN_PLAYER_FAILED: //播放出错，重新尝试播放
			{
				bypass_show_ctrl_->SetVisible(false);
				nim_comp::PlayerManager::GetInstance()->StartPlay(nbase::UTF16ToUTF8(this->rtmp_pull_url_), this->creater_id_);
				break;
			}
			case nim_comp::EN_PLAYER_STOPPED:
			{
				if (pull_stream_stopped_cb_.get() != NULL) //执行停止拉流后的任务，如：进入互动
					(*pull_stream_stopped_cb_.get())();
				break;
			}
			case nim_comp::EN_PLAYER_PLAYING:
				CloseMsgBox(L"pull_stream_failed"); //关闭拉流失败的提示框
				break;
			default:
				break;
			}
		};
		Post2UI(closure);
	}

	void ChatroomForm::SwitchPullStreamAndInteract(bool pull_stream)
	{
		if (master_)
		{
			assert(false);
			return;
		}

		if (!pull_stream) //切换为互动
		{
			if (livestreaming_) //已经在互动
			{
				assert(false);
				return;
			}

			//停止拉流
			pull_stream_stopped_cb_.reset(new nbase::WeakCallback<StdClosure>(ToWeakCallback([this]()
			{
				//开始互动
				StartLiveStream();
				pull_stream_stopped_cb_.reset();
			})));
			nim_comp::PlayerManager* player_manager = nim_comp::PlayerManager::GetInstance();
			if (!player_manager->StopPlay()) //当前没在拉流
				(*pull_stream_stopped_cb_.get())();
		}
		else //切换为拉流
		{
			StdClosure stop_closure = [this]()
			{
				//开始拉流
				nim_comp::PlayerManager* player_manager = nim_comp::PlayerManager::GetInstance();
				player_manager->StartPlay(nbase::UTF16ToUTF8(rtmp_pull_url_), creater_id_);
				video_show_ctrl_->SetAccount(creater_id_);
				interact_stopped_cb_.reset();
			};
			interact_stopped_cb_.reset(new nbase::WeakCallback<StdClosure>(ToWeakCallback(stop_closure)));
			if (livestreaming_) //正在互动，就停止互动
				StopLiveStream();
			else if (nim_comp::PlayerManager::GetInstance()->GetPlayerState() == EN_PLAYER_STATE::EN_PLAYER_STOPPING)
			{
				pull_stream_stopped_cb_.reset(new nbase::WeakCallback<StdClosure>(ToWeakCallback([this]()
				{
					//开始拉流
					nim_comp::PlayerManager* player_manager = nim_comp::PlayerManager::GetInstance();
					player_manager->StartPlay(nbase::UTF16ToUTF8(rtmp_pull_url_), creater_id_);
					video_show_ctrl_->SetAccount(creater_id_);
					pull_stream_stopped_cb_.reset();
				})));
			}
			else
				(*interact_stopped_cb_.get())();
		}
	}


}