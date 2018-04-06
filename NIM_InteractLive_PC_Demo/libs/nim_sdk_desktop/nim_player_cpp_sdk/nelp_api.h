/*
*	Author       biwei
*	Date         2016/10/19
*	Copyright    Hangzhou, Netease Inc.
*	Brief
*   Notice
*/

#ifndef _NELP_API_H_
#define _NELP_API_H_

#include "nelp_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \file */

/**
 * @brief 创建播放器实例
 * @param paLogpath:    日志文件路径   入参
 * @param phNLPService: 播放器实例     出参
 *
 * @return NELP_RET: NELP_OK 成功; NELP_ERR 失败
 */
EXPORTS_API NELP_RET Nelp_Create(const char *paLogpath, NELP_OUT _HNLPSERVICE *phNLPService);

/**
 * @brief 设置显示窗口句柄，用于显示
 * @param hNLPService:   播放器实例    入参
 * @param windowSurface: 显示窗口句柄  入参
 *
 * @return 无
 */
EXPORTS_API void     Nelp_SetDisPlay(_HNLPSERVICE hNLPService, void *windowSurface);

/**
 * @brief 初始化参数，包括播放地址、缓冲模式等
 * @param hNLPService: 播放器实例   入参
 * @param pstParam:    相关参数     入参
 *
 * @return  NELP_RET: NELP_OK 成功; NELP_ERR 失败
 */
EXPORTS_API NELP_RET Nelp_InitParam(_HNLPSERVICE hNLPService, ST_NELP_PARAM *pstParam);

/**
 * @brief 预处理播放器，包括拉流，解析等，如果设置成autoPlay，则预处理完成后会自动播放
 * @param hNLPService: 播放器实例   入参
 *
 * @return 无
 */
EXPORTS_API void     Nelp_PrepareToPlay(_HNLPSERVICE hNLPService);

/**
 * @brief 暂停后恢复播放
 * @param hNLPService: 播放器实例   入参
 *
 * @return 无
 */
EXPORTS_API void     Nelp_Start(_HNLPSERVICE hNLPService);

/**
 * @brief 暂停播放
 * @param hNLPService: 播放器实例   入参
 *
 * @return 无
 */
EXPORTS_API void     Nelp_Pause(_HNLPSERVICE hNLPService);

/**
 * @brief 关闭播放器并释放相关资源
 * @param hNLPService: 播放器实例   入参
 *
 * @return 无
 */
EXPORTS_API void     Nelp_Shutdown(_HNLPSERVICE hNLPService);

/**
 * @brief 静音功能
 * @param hNLPService: 播放器实例   入参
 * @param bIsMute: true 开启静音; false 静音恢复   入参
 *
 * @return 无
 */
EXPORTS_API void     Nelp_SetMute(_HNLPSERVICE hNLPService, bool bIsMute);

/**
 * @brief 获取当前播放位置
 * @param hNLPService: 播放器实例   入参
 *
 * @return 当前播放位置(单位: 秒 s)   < 0: 获取失败
 */
EXPORTS_API long     Nelp_GetCurrentPlaybackTime(_HNLPSERVICE hNLPService);

/**
 * @brief 获取文件总时长（仅适用于点播）
 * @param hNLPService: 播放器实例   入参
 *
 * @return 文件总时长(单位: 秒 s)   < 0: 获取失败
 */
EXPORTS_API long     Nelp_GetDuration(_HNLPSERVICE hNLPService);

/**
 * @brief 获取当前可播放的位置，即已缓存的位置（仅适用于点播）
 * @param hNLPService: 播放器实例   入参
 *
 * @return 当前可播放位置(单位: 秒 s)   < 0: 获取失败
 */
EXPORTS_API long     Nelp_GetPlayableDuration(_HNLPSERVICE hNLPService);

/**
 * @brief 指定到某一时间点播放（仅适用于点播）
 * @param hNLPService: 播放器实例              入参
 * @param lTime: 指定的播放时间点(单位: 秒 s)   入参
 *
 * @return NELP_RET: NELP_OK  成功    NELP_ERR  失败
 */
EXPORTS_API NELP_RET Nelp_SeekTo(_HNLPSERVICE hNLPService, long lTime);

/**
 * @brief 音量调节
 * @param hNLPService: 播放器实例                                   入参
 * @param fVolume: 音量大小 (范围: 0.0 ~ 1.0  0.0:静音   1.0:最大)   入参
 *
 * @return: NELP_RET: NELP_OK  成功    NELP_ERR  失败
 */
EXPORTS_API NELP_RET Nelp_SetVolume(_HNLPSERVICE hNLPService, float fVolume);

/**
 * @brief 获取SDK版本号
 * @param ppaVersion: SDK版本号    出参
 *
 * @return 无
 */
EXPORTS_API void     Nelp_GetSDKVersion(NELP_OUT char **ppaVersion);

/**
 * @brief 注册获取消息的回调, 用户需要实现回调函数cb来接收相关消息
 * @param hNLPService:  播放器实例      入参
 * @param pMessageCB:   获取消息的回调  入参

 * @return 无
 */
EXPORTS_API void     Nelp_RegisterMessageCB(_HNLPSERVICE hNLPService, P_NELP_MESSAGE_CB pMessageCB);


/**
 * @brief 注册获取视频帧数据的回调，用户需要实现回调函数cb来接收视频帧
 * @param hNLPService:   播放器实例      入参
 * @param bIsRender:     是否显示        入参
 * @param eMFormat:      回调的视频格式  入参
 * @param pVideFrameCB:  获取视频帧的回调    出参
 *
 * @return 无
 */
EXPORTS_API void     Nelp_RegisterGetVideoFrameCB(_HNLPSERVICE hNLPService, bool bIsRender, EN_NELP_MEDIA_FORMAT eMFormat, NELP_OUT P_NELP_VIDEO_FRAME_CB pVideFrameCB);

/**
* @brief 注册获取资源释放结束消息的回调(资源释放是异步的), 用户需要实现回调函数cb来接收相关消息
* @param hNLPService:  播放器实例      入参
* @param pReleaseCB:   获取资源释放结束消息的回调  入参
*
* @return 无
*/
EXPORTS_API void     Nelp_RegisterResourceReleaseSuccessCB(_HNLPSERVICE hNLPService, P_NELP_RESOURCE_RELEASE_SUCCESS_CB pReleaseCB);

/**
 * @brief 获取当前播放器的状态
 * @param hNLPService: 播放器实例     入参
 *
 * @return EN_NELP_PLAYBACK_STATE: 播放器状态   返回	EN_NELP_GET_PLAYBACK_STATE_FAILED 则表示获取失败
 */
EXPORTS_API EN_NELP_PLAYBACK_STATE Nelp_GetPlaybackState(_HNLPSERVICE hNLPService);

/**
* @brief 截图功能, 每次截图成功后返回一张图片
* @param hNLPService: 播放器实例    入参
* @param ePFormat: 图片格式         入参
*
* @return ST_NELP_PICTURE: 包含所截图片的原始数据和信息
*/
EXPORTS_API ST_NELP_PICTURE *Nelp_GetSnapshot(_HNLPSERVICE hNLPService, EN_NELP_PICTURE_FORMAT ePFormat);


#ifdef __cplusplus
}
#endif

#endif
