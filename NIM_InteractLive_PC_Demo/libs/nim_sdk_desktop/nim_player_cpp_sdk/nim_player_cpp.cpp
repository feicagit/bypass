/** @file nim_player_cpp.cpp
* @brief 播放器相关功能
* @copyright (c) 2015-2017, NetEase Inc. All rights reserved
* @author hzzhangjuan
* @date 2017/01/23
*/

#include "nim_player_cpp.h"
#include "base/win32/path_util.h"

namespace nim_player
{

HINSTANCE Player::instance_player_ = NULL;
_HNLPSERVICE  Player::phNLPService = NULL;
static const std::wstring kSdkPlayerDll = L"NELivePlayer.dll";

//reg callback
typedef void(*Nelp_RegisterMessageCB)(const _HNLPSERVICE phNLPService, P_NELP_MESSAGE_CB cb);
typedef NELP_RET(*Nelp_RegisterGetVideoFrameCB)(const _HNLPSERVICE phNLPService, const bool bIsRender, const EN_NELP_MEDIA_FORMAT eMFormat, P_NELP_VIDEO_FRAME_CB cb);
typedef void(*Nelp_RegisterResourceReleaseSuccessCB)(const _HNLPSERVICE phNLPService, P_NELP_RESOURCE_RELEASE_SUCCESS_CB cb);

typedef NELP_RET(*Nelp_Create)(const char *paLogpath, const _HNLPSERVICE *phNLPService);
typedef void(*Nelp_SetDisPlay)(const _HNLPSERVICE phNLPService, const void* wnd);
typedef NELP_RET(*Nelp_InitParam)(const _HNLPSERVICE phNLPService, const ST_NELP_PARAM *pstParam);
typedef void(*Nelp_PrepareToPlay)(const _HNLPSERVICE phNLPService);

typedef void(*Nelp_Start)(const _HNLPSERVICE phNLPService);
typedef void(*Nelp_Pause)(const _HNLPSERVICE phNLPService);
typedef void(*Nelp_Shutdown)(const _HNLPSERVICE phNLPService);
typedef void(*Nelp_SetMute)(const _HNLPSERVICE phNLPService, const bool bIsMute);

typedef long(*Nelp_GetCurrentPlaybackTime)(const _HNLPSERVICE phNLPService);
typedef long(*Nelp_GetDuration)(const _HNLPSERVICE phNLPService);
typedef long(*Nelp_GetPlayableDuration)(const _HNLPSERVICE phNLPService);
typedef NELP_RET(*Nelp_SeekTo)(const  _HNLPSERVICE phNLPService, const long ltime);
typedef NELP_RET(*Nelp_SetVolume)(const _HNLPSERVICE phNLPService, float fvolume);

typedef void(*Nelp_GetSDKVersion)(const  char **ppaVersion);
typedef EN_NELP_PLAYBACK_STATE(*Nelp_GetPlaybackState)(const _HNLPSERVICE phNLPService);
typedef ST_NELP_PICTURE* (*Nelp_GetSnapshot)(const _HNLPSERVICE phNLPService, EN_NELP_PICTURE_FORMAT ePFormat);


void Player::RegMessageCB(P_NELP_MESSAGE_CB cb)
{
	Nelp_RegisterMessageCB f_reg = Function<Nelp_RegisterMessageCB>("Nelp_RegisterMessageCB");
	return f_reg(phNLPService, cb);
}

NELP_RET Player::RegGetVideoFrameCB(bool bIsRender, EN_NELP_MEDIA_FORMAT eMFormat, P_NELP_VIDEO_FRAME_CB cb)
{
	Nelp_RegisterGetVideoFrameCB f_reg = Function<Nelp_RegisterGetVideoFrameCB>("Nelp_RegisterGetVideoFrameCB");
	return f_reg(phNLPService, bIsRender, eMFormat, cb);
}

void Player::RegResourceReleaseSuccessCB(P_NELP_RESOURCE_RELEASE_SUCCESS_CB cb)
{
	Nelp_RegisterResourceReleaseSuccessCB f_reg = Function<Nelp_RegisterResourceReleaseSuccessCB>("Nelp_RegisterResourceReleaseSuccessCB");
	return f_reg(phNLPService, cb);
}

void Player::LoadPlayerDll()
{
	if (instance_player_ == NULL)
	{
		std::wstring dll_path = nbase::win32::GetCurrentModuleDirectory() + L"nim_player\\" + kSdkPlayerDll;
		instance_player_ = ::LoadLibraryEx(dll_path.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
		assert(instance_player_);
	}
}

NELP_RET Player::Create(const std::string& paLogpath)
{
	assert(instance_player_);
	if (NULL == instance_player_)
		return NELP_ERR;
	if (phNLPService)
		return NELP_OK;
	Nelp_Create f_create = Function<Nelp_Create>("Nelp_Create");
	return f_create(paLogpath.c_str(), &phNLPService);
}

void Player::SetDisPlay(const void* wnd)
{
	Nelp_SetDisPlay f_set_display = Function<Nelp_SetDisPlay>("Nelp_SetDisPlay");
	f_set_display(phNLPService, wnd);
}

NELP_RET Player::InitParam(const ST_NELP_PARAM* pstParam)
{
	Nelp_InitParam f_init_param = Function<Nelp_InitParam>("Nelp_InitParam");
	return f_init_param(phNLPService, pstParam);
}

void Player::PrepareToPlay()
{
	Nelp_PrepareToPlay f_prepare_to_play = Function<Nelp_PrepareToPlay>("Nelp_PrepareToPlay");
	return f_prepare_to_play(phNLPService);
}

void Player::Start()
{
	Nelp_Start f_start = Function<Nelp_Start>("Nelp_Start");
	return f_start(phNLPService);
}

void Player::Pause()
{
	Nelp_Pause f_pause = Function<Nelp_Pause>("Nelp_Pause");
	return f_pause(phNLPService);
}

void Player::Shutdown()
{
	if (phNLPService != NULL)
	{
		Nelp_Shutdown f_shutdown = Function<Nelp_Shutdown>("Nelp_Shutdown");
		f_shutdown(phNLPService);
		phNLPService = NULL;
	}
}

void Player::ClosePlayer()
{
	Shutdown();
	::FreeLibrary(instance_player_);
	instance_player_ = NULL;
}

void Player::SetMute(bool bIsMute)
{
	Nelp_SetMute f_set_mute = Function<Nelp_SetMute>("Nelp_SetMute");
	return f_set_mute(phNLPService, bIsMute);
}

long Player::GetCurrentPlaybackTime()
{
	Nelp_GetCurrentPlaybackTime f_get_cur_pb_time = Function<Nelp_GetCurrentPlaybackTime>("Nelp_GetCurrentPlaybackTime");
	return f_get_cur_pb_time(phNLPService);
}

long Player::GetDuration()
{
	Nelp_GetDuration f_get_duration = Function<Nelp_GetDuration>("Nelp_GetDuration");
	return f_get_duration(phNLPService);
}

long Player::GetPlayableDuration()
{
	Nelp_GetPlayableDuration f_get_playable_duration = Function<Nelp_GetPlayableDuration>("Nelp_GetPlayableDuration");
	return f_get_playable_duration(phNLPService);
}

NELP_RET Player::SeekTo(const long lTime)
{
	Nelp_SeekTo f_seek_to = Function<Nelp_SeekTo>("Nelp_SeekTo");
	return f_seek_to(phNLPService, lTime);
}

NELP_RET Player::SetVolume(const float fVolume)
{
	Nelp_SetVolume f_set_volume = Function<Nelp_SetVolume>("Nelp_SetVolume");
	return f_set_volume(phNLPService, fVolume);
}

void Player::GetSDKVersion(const std::string& ppaVersion)
{
	const char *ppaversion_str = ppaVersion.c_str();
	Nelp_GetSDKVersion f_get_sdk_version = Function<Nelp_GetSDKVersion>("Nelp_GetSDKVersion");
	return f_get_sdk_version(&ppaversion_str);
}

EN_NELP_PLAYBACK_STATE Player::GetPlaybackState()
{
	Nelp_GetPlaybackState f_get_play_back_state = Function<Nelp_GetPlaybackState>("Nelp_GetPlaybackState");
	return f_get_play_back_state(phNLPService);
}

ST_NELP_PICTURE* Player::GetSnapshot(const EN_NELP_PICTURE_FORMAT ePFormat)
{
	Nelp_GetSnapshot f_get_snap_shot = Function<Nelp_GetSnapshot>("Nelp_GetSnapshot");
	return f_get_snap_shot(phNLPService, ePFormat);
}

}

