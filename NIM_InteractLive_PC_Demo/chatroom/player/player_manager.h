#pragma once

#include "module/video/video_manager.h"


namespace nim_comp
{

typedef enum {
	EN_PLAYER_IDLE,
	EN_PLAYER_PREPARING,
	EN_PLAYER_PLAYING,
	EN_PLAYER_STOPPING,
	EN_PLAYER_STOPPED,
	EN_PLAYER_FAILED,
} EN_PLAYER_STATE;

typedef std::function<void(EN_PLAYER_STATE)> PlayerStateCallback;

/** @class VideoManager
* @brief 播放管理器
* @copyright (c) 2016, NetEase Inc. All rights reserved
* @date 2016/09/18
*/
class PlayerManager : public nbase::SupportWeakCallback
{
public:
	SINGLETON_DEFINE(PlayerManager);

	/**
	* 播放过程中响应各种消息回调
	* @param[in] msg 消息
	@ return void	无返回值
	*/
	static void OnMessageCB(ST_NELP_MESSAGE msg);

	/**
	* 视频数据的回调
	* @param[in] frame 视频数据帧
	@ return void	无返回值
	*/
	static void OnGetVideoFrameCB(ST_NELP_FRAME *frame);

	/**
	* 资源释放成功的回调
	@ return void	无返回值
	*/
	static void OnResourceReleaseSuccessCB();

	/**
	* 加载视频播放器组件sdk动态库（应该在程序初始化时调用）
	* @return void	无返回值
	*/
	void LoadPlayerDll();

	/**
	* 清理UI组件
	* @return void	无返回值
	*/
	void CleanupPlayer();

	/**
	* 开始拉流播放，播放前执行Nelp_InitParam和Nelp_PrepareToPlay
	* @param[in] pull_rtmp_url 拉流地址
	* @return bool 播放是否成功
	*/
	bool StartPlay(const std::string& pull_rtmp_url,const std::string& creater_id);

	/**
	* 停止拉流播放
	* @return void 无返回值
	*/
	bool StopPlay();

	/**
	* 停止拉流播放
	* @return void 无返回值
	*/

	/**
	* 播放状态变化
	* @param in state 新状态
	* @return void 无返回值
	*/
	void PlayerStateChange(EN_PLAYER_STATE state);
	EN_PLAYER_STATE GetPlayerState(){ return player_state_; };

	/**
	* 注册播放状态变化回调
	* @param in callback 回调闭包
	* @return UnregisterCallback 返回反注册回调
	*/
	UnregisterCallback RegPlayerStateCallback(const PlayerStateCallback& callback);

private:
	PlayerManager();
	~PlayerManager();
	void StopPlayOnError();\
	void DoStatPlay(const std::string& pull_rtmp_url, const std::string& creater_id);
	void DoStopPlay();
	void OnStartPlayTimeout();
	void OnGetVideoFrameTimeout();
	friend void UIPlayerReleaseCallback();
	friend void UIPlayerMessageCallBack(ST_NELP_MESSAGE msg);
	friend void UIPlayerGetVideoFrameCB(const std::shared_ptr<ST_NELP_FRAME>& frame);

private:
	EN_PLAYER_STATE player_state_ = EN_PLAYER_IDLE; // player当前状态
	EN_PLAYER_STATE stopped_state_ = EN_PLAYER_STOPPED; // player完全关闭时的状态
	std::map<int, std::unique_ptr<PlayerStateCallback>> player_state_cb_list_;
	nbase::NLock player_lock_; //播放器同步锁
	nbase::WeakCallbackFlag start_play_timer_; //超时计时器
	nbase::WeakCallbackFlag video_frame_timer_; //视频数据超时计时器
};
}