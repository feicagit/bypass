/** @file nim_chatroom_cpp.h
* @brief NIM提供的播放器接口
* @copyright (c) 2015-2017, NetEase Inc. All rights reserved
* @author Oleg, Harrison
* @date 2015/12/29
*/

#ifndef _NIM_PLAYER_SDK_CPP_H_
#define _NIM_PLAYER_SDK_CPP_H_

#include <string>
#include <functional>
#include "nelp_define.h"
#include "nelp_type.h"

/**
* @namespace nim_player
* @brief 播放器
*/
namespace nim_player
{

/** @class ChatRoom
* @brief 播放器
*/
class Player
{

public:
	/** @fn  void RegMessageCB(MessgeCallbac& cb);
	* @brief 注册获取消息的回调, 用户需要实现回调函数cb来接收相关消息
	* @param cb[in]:   获取消息的回调
	* @return 无
	*/
	static void RegMessageCB(P_NELP_MESSAGE_CB cb);

	/**@fn  NELP_RET RegGetVideoFrameCB(_GetVideoFrameCallback& cb)
	* @brief 注册获取视频帧数据的回调，用户需要实现回调函数cb来接收视频帧
	* @param cb[in]:  获取视频帧的回调
	* @return  NELP_RET: NELP_OK 成功; NELP_ERR 失败
	*/
	static NELP_RET RegGetVideoFrameCB(bool bIsRender, EN_NELP_MEDIA_FORMAT eMFormat, P_NELP_VIDEO_FRAME_CB cb);

	/**@fn void  RegResourceReleaseSuccessCB(ResourceRealeaseCallback&cb)
	* @brief 注册获取资源释放结束消息的回调(资源释放是异步的), 用户需要实现回调函数cb来接收相关消息
	* @param cb[in]:   获取资源释放结束消息的回调
	* @return 无返回值
	*/
	static void  RegResourceReleaseSuccessCB(P_NELP_RESOURCE_RELEASE_SUCCESS_CB cb);

	/**@fnvoid  LoadPlayerDll( )
	* @brief 加载播放器sdk动态库
	* @param 无
	* @return 无返回值
	*/
	static void LoadPlayerDll();
	/**@fn NELP_RET Create(const std::string& paLogpath)
	* @brief 创建播放器实例
	* @param paLogpath[in]:    日志文件路径   入参
	* @return NELP_RET: NELP_OK 成功; NELP_ERR 失败
	*/
	static NELP_RET Create(const std::string& paLogpath);

	/**@fn  void SetDisPlay( const std::string&windowSurface)
	* @brief 设置显示窗口句柄，用于显示
	* @param windowSurface[in]: 显示窗口句柄
	* @return 无返回值
	*/
	static  void  SetDisPlay(const void* wnd);

	/**@fn NELP_RET InitParam( ST_NELP_PARAM *pstParam)
	* @brief 初始化参数，包括播放地址、缓冲模式等
	* @param pstParam[in]:    相关参数
	* @return  NELP_RET: NELP_OK 成功; NELP_ERR 失败
	*/
	static NELP_RET InitParam(const ST_NELP_PARAM* pstParam);

	/**@fn void  PrepareToPlay(c)
	* @brief 预处理播放器，包括拉流，解析等，如果设置成autoPlay，则预处理完成后会自动播放
	* @return 无
	*/
	static void PrepareToPlay();

	/**@fn void Start(_const std::string& json_extension = "")
	* @brief 暂停后恢复播放
	* @param[in] json_extension json扩展参数（备用，目前不需要）
	* @return 无返回值
	*/
	static void Start();

	/**@fn Pause(const std::string& json_extension = "")
	* @brief 暂停播放
	* @return 无返回值
	*/
	static void Pause();

	/** @fn Shutdown()
	* @brief 关闭当前播放视频
	* @return 无返回值
	*/
	static void Shutdown();

	/** @fn Shutdown()
	* @brief 关闭播放器并释放相关资源
	* @return 无返回值
	*/
	static void ClosePlayer();

	/**@fn SetMute( bool bIsMute)
	* @brief 静音功能
	* @param bIsMute[in]: true 开启静音; false 静音恢复   入参
	* @return 无返回值
	*/
	static void SetMute(bool bIsMute);

	/**@fn GetCurrentPlaybackTime()
	* @brief 获取当前播放位置（仅适用于点播）
	* @return 当前播放位置
	*/
	static long GetCurrentPlaybackTime();

	/**@fn long  GetDuration()
	* @brief 获取文件总时长（仅适用于点播）
	* @return 文件总时长
	*/
	static long GetDuration();

	/**@fn long  GetPlayableDuration()
	* @brief 获取当前可播放的位置，即已缓存的位置（仅适用于点播）
	* @return 当前可播放位置
	*/
	static long GetPlayableDuration();

	/**@fn SeekTo(const uint64_t lTime)
	* @brief 指定到某一时间点播放（仅适用于点播）
	* @param lTime[in]: 指定的播放时间点(单位: 毫秒 ms)
	* @return NELP_RET: NELP_OK  成功    NELP_ERR  失败
	*/
	static NELP_RET SeekTo(const long lTime);

	/**@fn  NELP_RET SetVolume(const float fVolume)
	* @brief 音量调节
	* @param fVolume: 音量大小 (范围: 0.0 ~ 1.0  0.0:静音   1.0:最大) 入参
	* @return: NELP_RET: NELP_OK  成功    NELP_ERR  失败
	*/
	static NELP_RET SetVolume(const float fVolume);

	/**@fn void GetSDKVersion(const std::string& ppaVersion)
	* @brief 获取SDK版本号
	* @param ppaVersion[in]: SDK版本号    出
	* @return 无返回值
	*/
	static void GetSDKVersion(const std::string& ppaVersion);

	/**@fn EN_NELP_PLAYBACK_STATE GetPlaybackState()
	* @brief 获取当前播放器的状态
	* @return EN_NELP_PLAYBACK_STATE: 播放器状态   返回	EN_NELP_GET_PLAYBACK_STATE_FAILED 则表示获取失败
	*/
	static EN_NELP_PLAYBACK_STATE GetPlaybackState();

	/**@fn ST_NELP_PICTURE *GetSnapshot(const EN_NELP_PICTURE_FORMAT ePFormat)
	* @brief 截图功能, 每次截图成功后返回一张图片
	* @param ePFormat[in]: 图片格式
	* @return ST_NELP_PICTURE: 包含所截图片的原始数据和信息
	*/
	static ST_NELP_PICTURE *GetSnapshot(const EN_NELP_PICTURE_FORMAT ePFormat);

private:
	template <typename F>
	static F Function(const char* function_name)
	{
		F f = (F) ::GetProcAddress(instance_player_, function_name);
		assert(f);
		return f;
	}

private:
	static HINSTANCE  instance_player_;
	static _HNLPSERVICE phNLPService;
};

}
#endif //_NIM_PLAYER_SDK_CPP_H_