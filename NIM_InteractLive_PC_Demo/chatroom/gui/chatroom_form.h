#pragma once
/*********************************** PC旁路直播Demo的一些说明 **************************************
************************************** 消息格式 ****************************************************
*主播进入房间的ext定的格式是.{type:xxxx} type：-1,1,2 ps.-1主播未在线,1是audio，2是vedio.
*
*连麦请求队列格式：      style:语音,视频
*                        state:等待，连接中，已连接
*                        info:  nick:
*                               avactar:avatar_default
*
*主播操作连麦通知格式：      Data:uid: 
							      nick:
*                                 avatar:
*                                 style:   1.audio,2.vedio
*                            type:2,3  ps.2是同意连麦 3是断开连麦
*
*自定义系统通知格式：  style:语音，视频
*                      command:通知类型,1,2,3,4,5.ps.见PushMicNotificationType
*                      info: nick:
*                            avatar:
*                      roomid:
*                
***************************************** 操作说明 ****************************************
**********************观众向主播连麦
*******************观众*******************     *******************主播*******************     
* 将自己加入到队列中                             
* 发送自定义通知给主播                          收到自定义通知
* ui操作                                        将请求连麦者加入内存队列
*                                               ui操作
*
*******************开始连麦
*******************主播*******************      *******************连麦者****************    ***其他观众***
* 选择-请求连麦者                                                    
* 发送自定义通知                                 收到同意连麦的自定义通知
*                                                加入多人会议  
* 收到连麦者加入会议的通知                       ui操作                      
* 全局发送自定义消息                                                                          收到自定义消息
* ui操作                                                                                      ui操作
*
*******************后进来的观众操作
* 进之前向服务器取聊天室队列信息
* 遍历队列，如果有观众的状态是连麦者
* 对状态是连麦者的观众进行ui操作 
*
*******************断开连麦
****************主播断开
******************主播********************       *******************连麦者******************   ***其他观众***
* 将连麦者从应用服务器中连麦队列中取出             
* 发送自定义消息通知全局                          退出音频房间，并切换到拉流状态
* 收到连麦者离开音频房间的通知                     
* 发送全局自定义消息                                                                            收到自定义消息
* ui操作                                          ui操作                                        ui操作
*
****************连麦者断开
*****************连麦者********************       ********************主播********************   ****其他观众*****
* 从应用服务器将自己从连麦队列中取出
* 退出多人会议                                      主播收到连麦者离开音频房间的通知
* 切换到拉流                                        发送全局自定义消息                           收到自定义消息
* 收到通知                                          ui操作                                       ui操作
* ui操作                                            
******************************************** 其他说明 *************************************************************
* 主播进入或者退出房间清空麦序队列 
* 主播加入频道成功之后，发送一个断开连麦的全局通知
***********************************/

#include "module/emoji/richedit_util.h"
#include "module/emoji/richedit_olecallback.h"
#include "module/video/video_frame_mng.h"
#include "player/player_manager.h"
#include "gui/video/helper/ui_bitmap_control.h"
#include "gui/video/helper/ui_barrage_control.h"
#include "gui/video/screen_capture_tool.h"
#include <time.h>
#include <atlbase.h>
#include <WTypes.h>
#include <shellapi.h>


namespace nim_chatroom
{

	typedef enum NTESLiveMicState
	{
		NTESLiveMicStateWaiting=1,    //队列等待
		NTESLiveMicStateConnecting, //连接中
		NTESLiveMicStateConnected,  //已连接
	}NTESLiveMicState;

	enum NTESAnchorOperate
	{
		NTESAnchorAgree = 2,
		kNTESAnchorRejectOrDisConnect, //主播拒绝或者是断开连接操作
	};

	enum PushMicNotificationType{
		kJoinQueue=1,     //加入连麦队列通知
		kExitQueue=2,     //退出连麦队列通知
		kConnectingMIC=3, //主播选择某人连麦
		kDisConnectingMIC=4,//主播断开连麦通知
		kRejectConnecting=5 //观众拒绝主播的连麦请求 ps.观众申请连麦->观众退出房间->主播连麦->观众发送系统通知拒绝主播连麦->主播从列表中删掉
	};

	enum NotifyType{
		kP2P,//点对点通知
		kTeam,//通知组内的全部人
	};

	enum InactionType{
		kUnLive = -1,//未开启直播
		kAudio = 1,//音频互动
		kVedio=2, //视频互动
	};

	enum BypassMemberListOpt{
		kAdd=0, //增加成员
		kUpdate=1,//更新成员
		kRemove=2,//删除成员
	};

	enum IdentityType{
		kLimitMember = -1,//受限用户
		kCustomMember=0,//普通
		kCreater=1, //创建者
		kManager=2, //管理者
	};

	enum GiftType
	{
		kUnKnown = -1,
		kGiftRose = 0,
		kGiftChocolate,
		kGiftBear,
		kGiftIceCream,
		kGiftRedHeart
	};

	struct BypassMemberInfo{
		NTESLiveMicState state;
		InactionType    type;
		string  nick;
	};

	struct GiftInfo
	{
		ui::Control* pImage;
		ui::Label*	pInfo;
		ui::Label*  pCount;
	};

class ChatroomForm : public nim_comp::WindowEx
{
public:
	ChatroomForm(__int64 room_id);
	~ChatroomForm();

	virtual std::wstring GetSkinFolder() override;
	virtual std::wstring GetSkinFile() override;
	virtual ui::Control* CreateControl(const std::wstring& pstrClass) override;
	virtual ui::UILIB_RESOURCETYPE GetResourceType() const;
	virtual std::wstring GetZIPFileName() const;

	virtual std::wstring GetWindowClassName() const override;
	virtual std::wstring GetWindowId() const override;
	virtual UINT GetClassStyle() const override;
	virtual void OnEsc(BOOL &bHandled) override;

	virtual void InitWindow() override;
	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	virtual void OnFinalMessage(HWND hWnd) override;

	void OnWndSizeMax(bool max);
	virtual bool Notify(ui::EventArgs* msg);
	virtual bool OnClicked(ui::EventArgs* msg);

public:
	void RequestEnter(const __int64 room_id);
	__int64 GetRoomId();
	std::string GetRoomSessionId();
	std::string GetRoomName();
	
	//普通消息回调
	void OnReceiveMsgCallback(const ChatRoomMessage& result);
	void OnEnterCallback(int error_code, const ChatRoomInfo& info, const ChatRoomMemberInfo& my_info);
	void OnGetChatRoomInfoCallback(__int64 room_id, int error_code, const ChatRoomInfo& info);
	void OnNotificationCallback(const ChatRoomNotification& notification);
	void OnGetMembersCallback(__int64 room_id, int error_code, const std::list<ChatRoomMemberInfo>& infos);
	void GetMsgHistoryCallback(__int64 room_id, int error_code, const std::list<ChatRoomMessage>& msgs);
	
	void OnChatRoomRequestEnterCallback(int error_code, const std::string& result);
	void OnRegLinkConditionCallback(__int64 room_id, const NIMChatRoomLinkCondition condition);

	void RequestRoomError();

	//设置直播模式：音频 1. 视频 2.
	void SetLivingModel(InactionType living_model);
	
	void SetAudience(bool audience){ audience_ = audience; }

	//直播

	bool NeedNotifyExt(){ return need_set_notify_ext_;}
	void SetNotifyExt(bool need){ need_set_notify_ext_ = need; }
	void CheckMaster();
	bool OnLiveStreamInit(bool camera);
	void SetRtmpUrl(std::string rtmp_url);
	void PopupMainMenu(POINT point);
	void ChangeCamera(bool open);
	void CloseDriver();

	bool StartLiveStream();
	void PushVideoStream();

	//callback
	void CreateRoomCallback(int code, __int64 channel_id, const std::string& json_extension);
	void JoinRoomCallback(int code, __int64 channel_id, const std::string& json_extension);
	void RoomConnectCallback(int code);
	
	//人员变化回调
	void RoomPeopleChangeCallback(std::string uid, bool join_type);


	//观众进之前应做的操作：需拉一把聊天室队列，并更新相关信息,主播初始化信息
	void InitChatRoomQueueInfo();
	void UpdateBypassMembersState(string uid, NTESLiveMicState micstate, InactionType type, BypassMemberListOpt opt,string nick);
	void DeleteBypassMembers();
	void AudienceAfterAnchorOpt(NTESAnchorOperate opt);//观众发起连麦主播端操作后观众应做的操作
	void SetChatRoomRoomName(string room_name){ room_name_ = room_name; }

	//拉流时错误操作
	void PullPlayError(int code, const std::string& msg);
	
private:
	void PaintVideo();
private:
	void OnUserInfoChange(const std::list<nim::UserNameCard> &uinfos);
	void OnUserPhotoReady(PhotoType type, const std::string& account, const std::wstring& photo_path);
	void OnHttoDownloadReady(HttpResourceType type, const std::string& account, const std::wstring& photo_path);

	void StartLiveStreamRet(bool ret);
	bool OnStartLiveStream(ui::EventArgs* msg);
	bool OnStopLiveStream(ui::EventArgs* msg);

	void StopLiveStream();
	void OnLsErrorCallback(int errCode);

private:
	bool OnEditEnter(ui::EventArgs* param);
	void OnBtnEmoji();
	bool OnSelChanged(ui::EventArgs* param);
	void OnEmotionSelected(std::wstring emo);
	void OnEmotionSelectedSticker(const std::wstring &catalog, const std::wstring &name);
	void OnEmotionClosed();
	bool OnSelOnlineMembers(ui::EventArgs* param);
	bool OnSelBypassMembers(ui::EventArgs* param);

	bool OnLinkClick(WPARAM wParam, LPARAM lParam);

	bool VChatMenuItemClick(ui::EventArgs* args);

	void ShowBypassPrompt();

	//主播操作连麦者菜单
	void ShowBypassOperateMenu(std::wstring &name);
	bool BypassAgreeMenuItemClick(ui::EventArgs* args);
	bool BypassRejectMenuItemClick(ui::EventArgs* args);
	bool OnMemberBypassMenu(ui::EventArgs* args);

private:
	void SetMemberAdmin(const std::string &id, bool is_admin);	//设置成员列表界面中某个成员的类型
	void SetMemberBlacklist(const std::string &id, bool is_black);
	void SetMemberMute(const std::string &id, bool is_mute);
	void SetMemberFixed(const std::string &id, bool is_fixed);
	void RemoveMember(const std::string &uid);
	void UpdateOnlineCount();	// 刷新在线人数
	void GetMembers();
private:
	void AddMsgItem(const ChatRoomMessage& result, bool is_history);
	void AddNotifyItem(const ChatRoomNotification& notification, bool is_history);
	void OnBtnSend();
	void AddText(const std::wstring &text, const std::wstring &sender_name, bool is_history);
	void AddNotify(const std::wstring &notify, bool is_history);
	void SendText(const std::string &text);
	void OnBtnJsb();
	void AddJsb(const int value, const std::wstring &sender_name, bool is_history);
	void SendJsb(const std::string &attach);

	void ShowWindowSelect(); 
	void SelectWndCb(HWND id, bool cut);
private:
	// 提供发送图片消息的例子
	void SendImage(const std::wstring &src);
	void OnUploadImageCallback(nim::NIMResCode res_code, const std::string& url, nim::IMImage img);


	//旁路直播
	void PopupInteractChoiceMenu(POINT point);
	void PopupCaptureChoiceMenu(POINT point);
	bool OnSingleWndClick(ui::EventArgs* param);
	bool OnWholeWndClick(ui::EventArgs* param);
	bool OnCameraWndClick(ui::EventArgs* param);
	
	//连麦者互动操作
	bool OnVideoInteractClick(ui::EventArgs* param);
	bool OnAudioInteractClick(ui::EventArgs* param);

	//向聊天室队列中添加连麦请求
	void RequestPushMicLink(std::string roomid, std::string uid, std::string ext,InactionType type);
	//从聊天室队列中取出连麦请求
	void RequestPopMicLink(std::string roomid, std::string uid);

	//请求聊天室地址
	void RequestAddress(std::string roomid, std::string uid);

	//旁路直播时给uid发送一条系统通知
	void SendSysNotifyMsg(PushMicNotificationType notifytype, string uid);

	//主播操作连麦
	void SendCustomMsg(NTESAnchorOperate opt,std::string uid, InactionType type);
	
	//主播进入频道发送全局消息
	void SendInitMsg();
	//更改互动操作状态
	void SetInteractState(InactionType interactiontype);

	//更新队列中的状态
	void UpdateChatRoomQueue(string uid, NTESLiveMicState state);

	//美颜操作
	void BeautyOperate();

	//查询聊天室状态信息
	void CheckChatroomStatus();
	void UpdateChatroomInfo(bool live);
	void UpdateChatroomInfoWithClose(bool live);

	//礼物
	void AddGiftInfo(const std::string& id, GiftType type, int nummber);
	void AddGiftNumbers(GiftType type, int number);//礼物数刷新

	void OnPullStreamPlayerStateChange(nim_comp::EN_PLAYER_STATE state);
	void SwitchPullStreamAndInteract(bool pull_stream); //互动直播切换拉流和互动
public:
	static const LPTSTR kClassName;
	static const int kAllowClose = 10;
	static const int kMaxPullRetryTime;

private:
	string          room_name_;
	__int64			room_id_;
	std::string		room_enter_token_;
	bool            need_set_notify_ext_;
	bool            audience_;
	
private:	
	//ui::Control*	header_icon_ = NULL; //标题栏头像
	//ui label
	ui::Label	*name_ = NULL; //标题栏姓名
	ui::Label	*ls_time_=NULL;
	ui::Label	*lb_gift_1_ = NULL;
	ui::Label	*lb_gift_2_ = NULL;
	ui::Label	*lb_gift_3_ = NULL;
	ui::Label	*lb_gift_4_ = NULL;
	ui::Label	*lb_gift_1_count_ = NULL;
	ui::Label	*lb_gift_2_count_ = NULL;
	ui::Label	*lb_gift_3_count_ = NULL;
	ui::Label	*lb_gift_4_count_ = NULL;

	//ui richedit
	ui::RichEdit	*bulletin_ = NULL;//公告	
	ui::RichEdit	*input_edit_ = NULL;
	ui::RichEdit	*msg_list_ = NULL;

	//ui option
	ui::Option	*option_bypass_members_ = NULL;
	ui::Option	*option_bypass_gift_ = NULL;

	//ui tabbox
	ui::TabBox	*list_tab_ = NULL;
	ui::TabBox	*ls_tab_ = NULL;

	//ui checkbox
	ui::CheckBox	*btn_face_ = NULL;

	//ui button
	ui::Button	*btn_send_ = NULL;
	ui::Button	*btn_start_ls_ = NULL;
	ui::Button	*btn_stop_ls_ = NULL;
	ui::Button	*btn_camera_set_ = NULL;
	ui::Button	*btn_mic_set_ = NULL;
	ui::Button	*btn_model_set_ = NULL;
	ui::Button	*btn_interact_set_ = NULL;
	ui::Button  *btn_audiohook_set = NULL;
	
	//ui listbox
	ui::ListBox	*online_members_list_ = NULL;
	ui::ListBox	*bypass_members_list_ = NULL;
	
	//ui control
	ui::Control	*bypass_prompt_ = NULL;
	ui::Control	*empty_members_list_ = NULL;
	ui::Control	*gift_png_1_ = NULL;
	ui::Control	*gift_png_2_ = NULL;
	ui::Control	*gift_png_3_ = NULL;
	ui::Control	*gift_png_4_ = NULL;

	//ui cbitmapcontrol
	ui::CBitmapControl	*video_show_ctrl_ = NULL;
	ui::CBitmapControl	*bypass_show_ctrl_ = NULL;

	//ui vbox
	ui::VBox	*gift_prompt_ = NULL;
	ui::VBox	*gift_info_ = NULL;
	ui::VBox	*gift_box_1_ = NULL;
	ui::VBox	*gift_box_2_ = NULL;
	ui::VBox	*gift_box_3_ = NULL;
	ui::VBox	*gift_box_4_ = NULL;

	IDropTarget	*droptarget_;
	nim_comp::IRichEditOleCallbackEx	*richeditolecallback_;
	HWND   cur_handle_id_;

private:
	//连麦相关
	bool	has_enter_;
	bool	has_add_creater_;	//是否在在线成员里添加了创建者
	bool	is_loading_history_;
	bool	master_;
	bool	livestreaming_;
	bool	is_audio_interact_;
	bool	is_vedio_interact_;
	bool	is_Camera_;

	std::string	creater_id_;	//创建者信息
	std::string	clicked_user_account_;	//被单击了的消息列表中的帐号
	std::string	kicked_user_account_;	//被踢出的帐号
	std::string	rtmp_url_;
	std::string	bypass_inact_account_; //menu菜单中操作的账号
	std::string	bypassingaccout_;//正在连麦的账号

	std::map<std::string, ChatRoomMemberInfo>	managers_list_;//管理员信息 <id,info>
	std::map<std::string, ChatRoomMemberInfo>	members_list_;//成员信息 <id,info>
	std::map<std::string, std::string>	nick_account_map_;//记录接受到的消息的昵称对应的帐号，方便通过昵称查到帐号
	std::map < std::string, BypassMemberInfo>	bypassmembers_list_;//旁路互动成员信息
	std::map<GiftType, GiftInfo>	gift_map_;

	time_t	time_refresh_;			//上一次刷新在线成员列表的时间
	AutoUnregister	unregister_cb;


	nim_comp::ScreenCaptureTool	*screen_tool_wnd_;
	nim_comp::VideoFrameMng video_frame_mng_;
	nim_comp::CaptureType cur_capute_;
	nbase::WeakCallbackFlag paint_video_timer_;
	nbase::WeakCallbackFlag push_video_timer_;
	nbase::WeakCallbackFlag check_chatroom_status_timer_;
	std::unique_ptr<nbase::WeakCallback<StdClosure>> pull_stream_stopped_cb_; //互动直播学生端拉流完全停止之后的任务（如：进入互动）
	std::unique_ptr<nbase::WeakCallback<StdClosure>> interact_stopped_cb_; //互动直播学生端互动完全结束之后的任务（如：开始拉流）

	int64_t channel_id_;
	int64_t start_time_stamp_;
	int64_t time_count_;
	int64_t	time_start_history;		//获取的最早的历史消息时间

	InactionType bypass_inact_type_;
	InactionType living_model_;

	int	gift_rose_count_;
	int	gift_bear_count_;
	int	gift_ice_count_;
	int	gift_cholocate_count_;


	
private: //美颜相关
	bool	is_open_beauty_;//是否已开启美颜

private: //拉流相关
	bool	is_pull_normal_;//拉流是否正常
	std::wstring	rtmp_pull_url_;//拉流地址
	NIMChatRoomLinkCondition chat_room_link_condition_;
	std::string pre_uid_;
};


}