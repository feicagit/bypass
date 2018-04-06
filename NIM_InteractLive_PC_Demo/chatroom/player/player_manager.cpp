#include "player_manager.h"
#include <sys/timeb.h>
#include "shared/modal_wnd/async_do_modal.h"
#include "gui/chatroom_form.h"
namespace nim_comp
{
	std::string host_id="";
	static const wchar_t* PlayerStates[6] = { L"EN_PLAYER_IDLE", L"EN_PLAYER_PREPARING", L"EN_PLAYER_PLAYING", L"EN_PLAYER_STOPPING", L"EN_PLAYER_STOPPED", L"EN_PLAYER_FAILED" };

	PlayerManager::PlayerManager()
	{
		LoadPlayerDll();
	}

	PlayerManager::~PlayerManager()
	{
		CleanupPlayer();
	}

	void UIPlayerMessageCallBack(ST_NELP_MESSAGE msg)
	{
		PlayerManager* player_manager = PlayerManager::GetInstance();
		switch (msg.iWhat)
		{
		case NELP_MSG_ERROR:
		{
			QLOG_ERR(L"PlayerManager::error happen !");
			player_manager->StopPlayOnError();
		}
			break;
		case NELP_MSG_PREPARED:
			QLOG_APP(L"PlayerManager::it is prepared to player");
			break;
		case NELP_MSG_VIDEO_SIZE_CHANGED:
			QLOG_APP(L"PlayerManager::video size changed");
			break;
		case NELP_MSG_BUFFERING_START:
			QLOG_APP(L"PlayerManager::buffering start");
			break;
		case NELP_MSG_BUFFERING_END:
			QLOG_APP(L"PlayerManager::buffering end");
			break;
		case NELP_MSG_COMPLETED:
			QLOG_APP(L"PlayerManager::play state complete");
			assert(false);
			break;
		case NELP_MSG_VIDEO_RENDERING_START:
			QLOG_APP(L"PlayerManager::video rendering start");;
			break;
		case NELP_MSG_AUDIO_RENDERING_START:
			QLOG_APP(L"PlayerManager::audio rendering start");
			break;
		case NELP_MSG_PLAYBACK_STATE_CHANGED:
			QLOG_APP(L"PlayerManager::player state change");
			break;
		case NELP_MSG_AUDIO_DEVICE_OPEN_FAILED:
		{
			QLOG_ERR(L"PlayerManager::audio device open fail");
			player_manager->StopPlayOnError();
		}
			break;
		case NELP_MSG_VIDEO_PARSE_ERROR:
		{
			QLOG_ERR(L"PlayerManager::video stream parse fail");
			player_manager->StopPlayOnError();
		}
			break;
		default:
			break;
		}
	}

	void PlayerManager::OnMessageCB(ST_NELP_MESSAGE msg)
	{
		Post2UI(nbase::Bind(&UIPlayerMessageCallBack, msg));
	}

	void PlayerManager::OnGetVideoFrameCB(ST_NELP_FRAME *frame)
	{
		timeb time_now;
		ftime(&time_now); // 秒数
		int64_t cur_timestamp = time_now.time * 1000 + time_now.millitm; // 毫秒
		char* userdata = (char*)frame->puaUsrData;
		int width = frame->iWidth;
		int height = frame->iHeight;
		int size = width * height * 3 / 2;
		Json::Value values;
		Json::FastWriter fw;
		values[nim::kNIMDeviceDataAccount] = host_id;
		std::string  str_creater_ = fw.write(values);
		nim_comp::VideoManager::GetInstance()->video_frame_mng_.AddVideoFrame(false, cur_timestamp, userdata, size, width, height, str_creater_, VideoFrameMng::Ft_I420);
	
		auto deleter = [](ST_NELP_FRAME* frame)
		{
			if (frame != NULL)
			{
				if (frame->puaUsrData != NULL)
					delete frame->puaUsrData;
				delete frame;
			}
		};
		std::shared_ptr<ST_NELP_FRAME> new_frame(new ST_NELP_FRAME({ frame->iWidth, frame->iHeight, NULL, frame->enMFormat }), deleter);
		size_t len = strlen((const char*)frame->puaUsrData);
		new_frame->puaUsrData = new unsigned char[len + 1];
		memset(new_frame->puaUsrData, 0, (len + 1) * sizeof(unsigned char));
		memcpy(new_frame->puaUsrData, frame->puaUsrData, len * sizeof(unsigned char));
		Post2UI(nbase::Bind(&UIPlayerGetVideoFrameCB, new_frame));
	}

	void UIPlayerReleaseCallback()
	{
		QLOG_APP(L"Player released.");
		PlayerManager::GetInstance()->PlayerStateChange(PlayerManager::GetInstance()->stopped_state_);
	}

	void PlayerManager::OnResourceReleaseSuccessCB()
	{
		Post2UI(nbase::Bind(&UIPlayerReleaseCallback));
	}

	void PlayerManager::LoadPlayerDll()
	{
		nbase::NAutoLock auto_lock(&player_lock_);
		nim_player::Player::LoadPlayerDll();
	}

	void PlayerManager::CleanupPlayer()
	{
		if (player_state_ == EN_PLAYER_PREPARING || player_state_ == EN_PLAYER_PLAYING)
		{
			player_state_ = EN_PLAYER_STOPPING;
			stopped_state_ = EN_PLAYER_STOPPED;
		}
		nbase::NAutoLock auto_lock(&player_lock_);
		nim_player::Player::ClosePlayer();
	}

	bool PlayerManager::StartPlay(const std::string& pull_rtmp_url, const std::string& create_id)
	{
		if (player_state_ == EN_PLAYER_PLAYING || player_state_ == EN_PLAYER_PREPARING || player_state_ == EN_PLAYER_STOPPING)
		{
			QLOG_APP(L"Player current state is {0}.") << PlayerStates[player_state_];
			return false;
		}
		PlayerStateChange(EN_PLAYER_PREPARING);
		QLOG_APP(L"PlayerManager::StartPlay");
		nbase::ThreadManager::PostTask(kThreadLivePlaying, nbase::Bind(&PlayerManager::DoStatPlay, this, pull_rtmp_url,create_id));
		return true;
	}

	void  PlayerManager::DoStatPlay(const std::string& pull_rtmp_url, const std::string& creater_id)
	{
		nbase::ThreadManager::PostDelayedTask(start_play_timer_.ToWeakCallback(nbase::Bind(&PlayerManager::OnStartPlayTimeout, this)), nbase::TimeDelta::FromSeconds(30)); //设置30秒超时
		host_id = creater_id;
		std::wstring log_dir = QPath::GetNimAppDataDir() + L"live_player\\";
		if (!nbase::FilePathIsExist(log_dir, true))
			nbase::win32::CreateDirectoryRecursively(log_dir.c_str());
		nbase::NAutoLock auto_lock(&player_lock_);
		if (NELP_OK != nim_player::Player::Create(nbase::UTF16ToUTF8(log_dir))) // 创建播放器实例
		{
			QLOG_ERR(L"Create player instance error."); // 创建失败
			PlayerStateChange(EN_PLAYER_FAILED); //直接修改状态
			return;
		}
		nim_player::Player::RegMessageCB(&PlayerManager::OnMessageCB); // 注册获取消息的回调
		nim_player::Player::RegGetVideoFrameCB(false, EN_YUV420, &PlayerManager::OnGetVideoFrameCB); // 注册获取视频数据的回调
		nim_player::Player::RegResourceReleaseSuccessCB(&PlayerManager::OnResourceReleaseSuccessCB); // 注册接收资源释放成功的回调

		ST_NELP_PARAM *pstNelpParam = (ST_NELP_PARAM *)malloc(sizeof(ST_NELP_PARAM)); // 设置初始化参数，包括播放地址，缓冲类型等
		if (pstNelpParam != NULL) {
			pstNelpParam->paPlayUrl = (char*)pull_rtmp_url.c_str();
			pstNelpParam->bAutoPlay = true;
			pstNelpParam->enBufferStrategy = EN_NELP_LOW_DELAY;
		}
		NELP_RET ret = nim_player::Player::InitParam(pstNelpParam);
		free(pstNelpParam); // 释放分配的结构体空间
		if (NELP_OK != ret){ // 初始化相关参数
			QLOG_ERR(L"Init player param error."); // 初始化相关参数失败
			StopPlayOnError();
			return;
		}
		nim_player::Player::PrepareToPlay(); // 预处理播放
		nim_player::Player::Start(); // 开始播放
		return;
	}

	void PlayerManager::OnStartPlayTimeout()
	{
		if (player_state_ == EN_PLAYER_IDLE) //30秒还没有播放成功，视为超时
		{
			QLOG_APP(L"Start play timeout.");
			StopPlayOnError();
		}
	}

	 void UIPlayerGetVideoFrameCB(const std::shared_ptr<ST_NELP_FRAME>& frame)
	{
		PlayerManager* player_manager = PlayerManager::GetInstance();
		if (player_manager->player_state_ == EN_PLAYER_PREPARING)
			player_manager->PlayerStateChange(EN_PLAYER_PLAYING);
		if (player_manager->player_state_ == EN_PLAYER_PLAYING)
		{
			player_manager->video_frame_timer_.Cancel();
			auto timeout_cb = player_manager->video_frame_timer_.ToWeakCallback(nbase::Bind(&PlayerManager::OnGetVideoFrameTimeout, player_manager));
			nbase::ThreadManager::PostDelayedTask(timeout_cb, nbase::TimeDelta::FromSeconds(30)); //超时判断
		}
	}

	 void PlayerManager::OnGetVideoFrameTimeout()
	 {
		 if (player_state_ == EN_PLAYER_PLAYING) //30秒之内没有收到视频帧了，视为超时
		 {
			 QLOG_APP(L"Get video frame timeout.");
			 StopPlayOnError();
		 }
	 }

	bool PlayerManager::StopPlay()
	{
		if (player_state_  && player_state_ != EN_PLAYER_PLAYING)
		{
			QLOG_APP(L"Player current state is {0}.") << PlayerStates[player_state_];
			return false;
		}
		QLOG_APP(L"PlayerManager::StopPlay");
		PlayerStateChange(EN_PLAYER_STOPPING);
		stopped_state_ = EN_PLAYER_STOPPED; //正常停止
		DoStopPlay();
		//nbase::ThreadManager::PostTask(kThreadLivePlaying, nbase::Bind(&PlayerManager::DoStopPlay, this));
		return true;
	}

	void PlayerManager::StopPlayOnError()
	{
		if (player_state_ != EN_PLAYER_PREPARING && player_state_ != EN_PLAYER_PLAYING)
		{
			QLOG_APP(L"Player current state is {0}.") << PlayerStates[player_state_];
			return;
		}
		QLOG_APP(L"PlayerManager::StopPlayOnError");
		PlayerStateChange(EN_PLAYER_STOPPING);
		stopped_state_ = EN_PLAYER_FAILED; //因错误而停		
		DoStopPlay();
		//nbase::ThreadManager::PostTask(kThreadLivePlaying, nbase::Bind(&PlayerManager::DoStopPlay, this));
	}

	void PlayerManager::DoStopPlay()
	{
		nbase::NAutoLock auto_lock(&player_lock_);
		nim_player::Player::Shutdown();
	}

	void PlayerManager::PlayerStateChange(EN_PLAYER_STATE state)
	{
		if (player_state_ != state)
		{
			QLOG_APP(L"PlayerManager::PlayerStateChange: {0}.") << PlayerStates[state];
			player_state_ = state;
			start_play_timer_.Cancel(); //状态有变化就取消超时处理
			video_frame_timer_.Cancel(); //状态有变化就取消超时处理
			//assert(nbase::MessageLoop::current()->ToUIMessageLoop());
			for (auto& it : player_state_cb_list_)
				(*(it.second))(state);
		}
	}

	UnregisterCallback PlayerManager::RegPlayerStateCallback(const PlayerStateCallback& callback)
	{
		//assert(nbase::MessageLoop::current()->ToUIMessageLoop());
		PlayerStateCallback* new_callback = new PlayerStateCallback(callback);
		int cb_id = (int)new_callback;
		player_state_cb_list_[cb_id].reset(new_callback);
		auto cb = ToWeakCallback([this, cb_id]() {
			player_state_cb_list_.erase(cb_id);
		});
		return cb;
	}
}
