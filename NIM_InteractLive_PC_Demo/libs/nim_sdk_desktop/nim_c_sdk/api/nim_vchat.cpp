// Copyright (c) 2011, NetEase Inc. All rights reserved.
//
// Author: Gq <gaoqi@yixin.im>
// Date: 2015/4/30
//

#include "nim_vchat.h"
#include "nim_sdk_util.h"
#include "nim_lib/vchat/nim_vchat_manager.h"

extern"C"
{
//NIM VCHAT初始化
bool nim_vchat_init(const char *json_extension)
{
	LOG_APP("[api]vchat.call init");
	return nim::NimVChatManager::GetInstance()->VChatInit(json_extension);
}
//NIM VCHAT清理
void nim_vchat_cleanup(const char *json_extension)
{
	LOG_APP("[api]vchat.call cleanup");
	nim::NimVChatManager::GetInstance()->VChatCleanup();
}

//设置回调
void nim_vchat_set_cb_func(nim_vchat_cb_func cb, const void *user_data)
{
	LOG_APP("[api]vchat.reg cb");
	return nim::NimVChatManager::GetInstance()->SetVChatCbFunc(cb, user_data);
}
//启动通话
//json_info 见nim_vchat_def.h
bool nim_vchat_start(NIMVideoChatMode mode, const char *apns_text, const char *custom_info, const char *json_extension, const void *user_data)
{
	LOG_APP("[api]vchat.call start, mode:%d", mode);
	return nim::NimVChatManager::GetInstance()->StartChat(mode, PCharToString(apns_text), PCharToString(custom_info), PCharToString(json_extension));
}
//设置通话模式
bool nim_vchat_set_talking_mode(NIMVideoChatMode mode, const char *json_extension)
{
	LOG_APP("[api]vchat.call set talking mode, mode:%d", mode);
	return nim::NimVChatManager::GetInstance()->SetTalkingMode(mode, PCharToString(json_extension));
}
//回应邀请
bool nim_vchat_callee_ack(int64_t channel_id, bool accept, const char *json_extension, const void *user_data)
{
	LOG_APP("[api]vchat.call scallee ack, accept:%d", accept);
	return nim::NimVChatManager::GetInstance()->VChatCalleeAck(channel_id, accept, PCharToString(json_extension));
}
//通话控制
bool nim_vchat_control(int64_t channel_id, NIMVChatControlType type, const char *json_extension, const void *user_data)
{
	LOG_APP("[api]vchat.call control, type:%d", type);
	return nim::NimVChatManager::GetInstance()->VChatControl(channel_id, type);
}

void nim_vchat_set_viewer_mode(bool viewer)
{
	LOG_APP("[api]vchat.call set viewer mode, viewer:%d", viewer);
	nim::NimVChatManager::GetInstance()->SetViewerMode(viewer);
}

bool nim_vchat_get_viewer_mode()
{
	LOG_APP("[api]vchat.call get viewer mode");
	return nim::NimVChatManager::GetInstance()->IsViewerMode();
}

void nim_vchat_set_audio_mute(bool muted)
{
	LOG_APP("[api]vchat.call set audio mute, mute:%d", muted);
	nim::NimVChatManager::GetInstance()->SetAudioMute(muted);
}

bool nim_vchat_audio_mute_enabled()
{
	LOG_APP("[api]vchat.call get audio mute");
	return nim::NimVChatManager::GetInstance()->IsAudioMute();
}

void nim_vchat_set_rotate_remote_video(bool rotate)
{
	LOG_APP("[api]vchat.call set rotate remote video, rotate:%d", rotate);
	nim::NimVChatManager::GetInstance()->SetRotateRemoteVideo(rotate);
}

bool nim_vchat_rotate_remote_video_enabled()
{
	LOG_APP("[api]vchat.call get rotate remote video");
	return nim::NimVChatManager::GetInstance()->IsRotateRemoteVideo();
}
void nim_vchat_set_video_frame_scale(NIMVChatVideoFrameScaleType type)
{
	LOG_APP("[api]vchat.call set video frame scale type");
	nim::NimVChatManager::GetInstance()->SetVideoFrameScaleType(type);
}

int nim_vchat_get_video_frame_scale_type()
{
	LOG_APP("[api]vchat.call get video frame scale type");
	return nim::NimVChatManager::GetInstance()->GetVideoFrameScaleType();
}

void nim_vchat_set_member_in_blacklist(const char *uid, bool add, bool audio, const char *json_extension, nim_vchat_opt_cb_func cb, const void *user_data)
{
	LOG_APP("[api]vchat.call set member in blacklist");
	nim::NimVChatManager::GetInstance()->SetMemberInBlacklist(PCharToString(uid), add, audio, PCharToString(json_extension), cb, user_data);
}

void nim_vchat_start_record(const char *path, const char *json_extension, nim_vchat_mp4_record_opt_cb_func cb, const void *user_data)
{
	LOG_APP("[api]vchat.call start record");
	nim::NimVChatManager::GetInstance()->StartRecord(path, json_extension, cb, user_data);
}

void nim_vchat_stop_record(const char *json_extension, nim_vchat_mp4_record_opt_cb_func cb, const void *user_data)
{
	LOG_APP("[api]vchat.call stop record");
	nim::NimVChatManager::GetInstance()->StopRecord(json_extension, cb, user_data);
}
void nim_vchat_start_audio_record(const char *path, const char *json_extension, nim_vchat_audio_record_opt_cb_func cb, const void *user_data)
{
	LOG_APP("[api]vchat.call start audio record");
	nim::NimVChatManager::GetInstance()->StartAudioRecord(path, json_extension, cb, user_data);
}

void nim_vchat_stop_audio_record(const char *json_extension, nim_vchat_audio_record_opt_cb_func cb, const void *user_data)
{
	LOG_APP("[api]vchat.call stop audio record");
	nim::NimVChatManager::GetInstance()->StopAudioRecord(json_extension, cb, user_data);
}

//结束通话
void nim_vchat_end(const char *json_extension)
{
	LOG_APP("[api]vchat.call end");
	nim::NimVChatManager::GetInstance()->EndChat(PCharToString(json_extension));
}

void nim_vchat_create_room(const char *room_name, const char *custom_info, const char *json_extension, nim_vchat_opt2_cb_func cb, const void *user_data)
{
	LOG_APP("[api]vchat.call create room");
	nim::NimVChatManager::GetInstance()->CreateChannel(PCharToString(room_name), PCharToString(custom_info), PCharToString(json_extension), cb, user_data);
}

bool nim_vchat_join_room(NIMVideoChatMode mode, const char *room_name, const char *json_extension, nim_vchat_opt2_cb_func cb, const void *user_data)
{
	LOG_APP("[api]vchat.call join room");
	return nim::NimVChatManager::GetInstance()->JoinChannel(mode, PCharToString(room_name), PCharToString(json_extension), cb, user_data);
}

void nim_vchat_set_video_quality(int video_quality, const char *json_extension, nim_vchat_opt_cb_func cb, const void *user_data)
{
	LOG_APP("[api]vchat.call set video quality, qua:%d", video_quality);
	nim::NimVChatManager::GetInstance()->SetVideoQuality(video_quality, json_extension, cb, user_data);
}
void nim_vchat_set_frame_rate(NIMVChatVideoFrameRate frame_rate, const char* json_extension, nim_vchat_opt_cb_func cb, const void *user_data)
{
	LOG_APP("[api]vchat.call set video frame rate, type:%d", frame_rate);
	nim::NimVChatManager::GetInstance()->SetVideoFrameRate(frame_rate, json_extension, cb, user_data);
}

void nim_vchat_set_video_bitrate(int video_bitrate, const char *json_extension, nim_vchat_opt_cb_func cb, const void *user_data)
{
	LOG_APP("[api]vchat.call set video bitrate, qua:%d", video_bitrate);
	nim::NimVChatManager::GetInstance()->SetVideoBitrate(video_bitrate, json_extension, cb, user_data);
}

void nim_vchat_set_custom_data(bool custom_audio, bool custom_video, const char *json_extension, nim_vchat_opt_cb_func cb, const void *user_data)
{
	LOG_APP("[api]vchat.call set custom data, audio:%d, video:%d", custom_audio, custom_video);
	nim::NimVChatManager::GetInstance()->SetCustomData(custom_audio, custom_video, json_extension, cb, user_data);
}

void nim_vchat_update_rtmp_url(const char *rtmp_url, const char *json_extension, nim_vchat_opt_cb_func cb, const void *user_data)
{
	LOG_APP("[api]vchat.call update rtmp url");
	nim::NimVChatManager::GetInstance()->UpdateRtmpUrl(rtmp_url, json_extension, cb, user_data);
}
void nim_vchat_select_video_adaptive_strategy(NIMVChatVideoEncodeMode mode, const char *json_extension, nim_vchat_opt_cb_func cb, const void *user_data)
{
	LOG_APP("[api]vchat.call select video adaptive strategy %d", mode);
	nim::NimVChatManager::GetInstance()->SelectVideoAdaptiveStrategy(mode, json_extension, cb, user_data);
}

/** @enum NIMVChatSetStreamingModeCode 设置推流模式返回码  */
enum NIMVChatSetStreamingModeCode{
	kNIMVChatBypassStreamingInvalid					= 0,			/**< 无效的操作 */
	kNIMVChatBypassStreamingSuccess					= 200,			/**< 设置成功 */
	kNIMVChatBypassStreamingErrorExceedMax			= 202,			/**< 超过最大允许直播节点数量 */
	kNIMVChatBypassStreamingErrorHostNotJoined		= 203,			/**< 必须由主播第一个开启直播 */
	kNIMVChatBypassStreamingErrorServerError		= 204,			/**< 互动直播服务器错误 */
	kNIMVChatBypassStreamingErrorOtherError			= 205,			/**< 互动直播其他错误 */
	kNIMVChatBypassStreamingErrorNoResponse			= 404,			/**< 互动直播服务器没有响应 */
	kNIMVChatBypassStreamingErrorReconnecting		= 405,			/**< 重连过程中无法进行相关操作，稍后再试 */
	kNIMVChatBypassStreamingErrorTimeout			= 408,			/**< 互动直播设置超时 */
};
/** @fn void nim_vchat_set_streaming_mode(bool streaming, const char* json_info, nim_vchat_opt_cb_func cb, const void *user_data)
* NIM 推流开关
* @param[in] streaming  是否加入推流
* @param[in] json_info  无效备用
* @param[in] cb 结果回调见nim_vchat_def.h， code返回参见NIMVChatSetStreamingModeCode
* @param[in] user_data APP的自定义用户数据，SDK只负责传回给回调函数cb，不做任何处理！
* @return void 无返回值
* @note 错误码	0:成功
*				11403:无效的操作
*/
//NIM_SDK_DLL_API void nim_vchat_set_streaming_mode(bool streaming, const char* json_info, nim_vchat_opt_cb_func cb, const void *user_data);

void nim_vchat_set_streaming_mode(bool streaming, const char* json_info, nim_vchat_opt_cb_func cb, const void *user_data)
{
	LOG_APP("[api]vchat.call set streaming mode");
	nim::NimVChatManager::GetInstance()->SetStreamingMode(streaming, json_info, cb, user_data);
}
uint64_t nim_vchat_net_detect(const char *json_extension, nim_vchat_opt_cb_func cb, const void *user_data)
{
	LOG_APP("[api]vchat.call net detect");
	return nim::NimVChatManager::GetInstance()->AddNetDetectTask(json_extension, cb, user_data);
}

void nim_vchat_set_uid_picture_as_main(const char *uid, const char* json_info, nim_vchat_opt_cb_func cb, const void* user_data)
{
	LOG_APP("[api] vchat.call set uid [%s] picture as main.", uid);
	return nim::NimVChatManager::GetInstance()->SetUidPictureAsMain(PCharToString(uid), json_info, cb, user_data);
}

};