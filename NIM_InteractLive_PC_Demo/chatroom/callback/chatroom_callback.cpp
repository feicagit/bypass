#include "chatroom_callback.h"
#include "gui/chatroom_form.h"


namespace nim_chatroom
{
	void CustomSysMessageToIMMessage(const nim::SysMessage &sys_msg, nim::IMMessage &msg)
	{
		//无法对等
		//msg.attach_ = sys_msg.apns_text_;

		//这个方法只能再要显示在ui前调用
		msg.receiver_accid_ = sys_msg.receiver_accid_;
		msg.sender_accid_ = sys_msg.sender_accid_;
		msg.timetag_ = sys_msg.timetag_;
		msg.content_ = sys_msg.content_;
		msg.attach_ = sys_msg.attach_;
		msg.readonly_server_id_ = sys_msg.id_;
		msg.msg_setting_.server_history_saved_ = sys_msg.msg_setting_.need_offline_;
		msg.rescode_ = sys_msg.rescode_;
		msg.feature_ = sys_msg.feature_;
		msg.session_type_ = sys_msg.type_ == nim::kNIMSysMsgTypeCustomP2PMsg ? nim::kNIMSessionTypeP2P : nim::kNIMSessionTypeTeam;
	}

	void ChatroomCallback::RejectInteractionRequest(int style, string nick, string room_id, string sender_id_,string receiver_id_)
	{
		Json::FastWriter fs;
		Json::Value value;
		Json::Value json;
		json["style"] = style;
		//加入连麦队列通知
		json["command"] = kDisConnectingMIC;
		json["info"]["nick"] = nick;
		json["info"]["avatar"] = "avatar_default";
		//json["info"] = json_info;
		json["roomid"] = room_id;
		nim::SysMessage msg;
		msg.receiver_accid_ = sender_id_;
		msg.sender_accid_ =receiver_id_;
		msg.msg_setting_.need_offline_ = nim::BS_FALSE;
		msg.attach_ = fs.write(json);
		msg.type_ = nim::kNIMSysMsgTypeCustomP2PMsg;
		msg.client_msg_id_ = QString::GetGUID();
		msg.timetag_ = 1000 * nbase::Time::Now().ToTimeT();
		nim::SystemMsg::SendCustomNotificationMsg(msg.ToJsonString());
	}

	void ChatroomCallback::UIReceiveSysmsgCallback(nim::SysMessage& msg)
	{
	   if (msg.type_ == nim::kNIMSysMsgTypeCustomP2PMsg||msg.type_==nim::kNIMSysMsgTypeCustomTeamMsg)
		{
			nim::IMMessage immsg;
			CustomSysMessageToIMMessage(msg, immsg);
			//if (msg.msg_setting_.need_offline_ != nim::BS_TRUE)//只在线
			{
				if (msg.type_ == nim::kNIMSysMsgTypeCustomP2PMsg)
				{
					Json::Value json;
					Json::Reader reader;
					if (reader.parse(msg.attach_, json) && json.isObject())
					{
						PushMicNotificationType type = (PushMicNotificationType)(json["command"].asInt());
						switch (type)
						{//主播才会收到
						case nim_chatroom::kJoinQueue:
							if (json.isMember("info"))
							{
								//{"style":1, "info" : {"nick":"123", "avatar" : "avatar_default"}, "command" : 1, "roomid" : "3599"}
								std::string bypassid = msg.sender_accid_;
								//std::string name = json["info"]["name"].asString();
								int  style = json["style"].asInt();
								std::string room_id = json["roomid"].asString();
								string nick = json["info"]["nick"].asString();
								//主播发送一条消息给被连接者
								nim_chatroom::ChatroomForm* chat_form = static_cast<ChatroomForm*>(nim_ui::WindowsManager::GetInstance()->GetWindow(ChatroomForm::kClassName, nbase::UTF8ToUTF16(room_id)));
								if (chat_form == nullptr)
								{
									RejectInteractionRequest(style, nick, room_id,msg.sender_accid_,msg.receiver_accid_);
									return;
								}
								chat_form->UpdateBypassMembersState(msg.sender_accid_, NTESLiveMicStateWaiting, (InactionType)style, nim_chatroom::kAdd, nick);

							}
							break;
						case nim_chatroom::kExitQueue:
						{
							std::string bypassid = msg.sender_accid_;
							//std::string name = json["info"]["name"].asString();
							int  style = json["style"].asInt();
							std::string room_id = json["roomid"].asString();
							string nick = json["info"]["nick"].asString();
							//主播发送一条消息给被连接者
							nim_chatroom::ChatroomForm* chat_form = static_cast<ChatroomForm*>(nim_ui::WindowsManager::GetInstance()->GetWindow(ChatroomForm::kClassName, nbase::UTF8ToUTF16(room_id)));
							if (chat_form == nullptr)
							{
								return;
							}
							chat_form->UpdateBypassMembersState(msg.sender_accid_, NTESLiveMicStateWaiting, (InactionType)style, nim_chatroom::kRemove, nick);

						}
						break;
						case nim_chatroom::kConnectingMIC:
							//{"command":3,"roomid":"4494"}
						{
							string room_id = json["roomid"].asString();
							int command = json["command"].asInt();
							//主播发送一条消息给被连接者
							nim_chatroom::ChatroomForm* chat_form = static_cast<ChatroomForm*>(nim_ui::WindowsManager::GetInstance()->GetWindow(ChatroomForm::kClassName, nbase::UTF8ToUTF16(room_id)));
							if (chat_form == nullptr)
							{
								return;
							}
							chat_form->AudienceAfterAnchorOpt(NTESAnchorAgree);
						}
						break;
						case nim_chatroom::kDisConnectingMIC:
						{
							std::string bypassid = msg.sender_accid_;
							//std::string name = json["info"]["name"].asString();
							int  style = json["style"].asInt();
							string room_id = json["roomid"].asString();
							std::string nick = json["info"]["nick"].asString();
							//主播发送一条消息给被连接者
							nim_chatroom::ChatroomForm* chat_form = static_cast<ChatroomForm*>(nim_ui::WindowsManager::GetInstance()->GetWindow(ChatroomForm::kClassName, nbase::UTF8ToUTF16(room_id)));
							
							if (chat_form == nullptr)
							{
								return;
							}
							chat_form->AudienceAfterAnchorOpt(kNTESAnchorRejectOrDisConnect);

						}
						case nim_chatroom::kRejectConnecting:
						{
							std::string bypassid = msg.sender_accid_;
							//std::string name = json["info"]["name"].asString();
							int  style = json["style"].asInt();
							int64_t room_id = json["roomid"].asInt64();
							string nick = json["info"]["nick"].asString();
							nim_chatroom::ChatroomForm* chat_form = static_cast<ChatroomForm*>(nim_ui::WindowsManager::GetInstance()->GetWindow(ChatroomForm::kClassName, nbase::Int64ToString16(room_id)));
							if (chat_form != nullptr)
								chat_form->UpdateBypassMembersState(msg.sender_accid_, NTESLiveMicStateWaiting, (InactionType)style, nim_chatroom::kRemove, nick);
							ChatRoom::QueuePollAsync(room_id, msg.sender_accid_, nullptr);
#ifdef _DEBUG
							ChatRoom::QueueListCallback cb = [](__int64 room_id, int error_code, const ChatRoomQueue& queue)
							{
								if (error_code == 200)
								{
									QLOG_ERR(L"QueueListAsync count. count: {0}.") << queue.size();
									auto it = queue.begin();
									for (; it != queue.end(); it++)
									{
										QLOG_ERR(L"QueueListAsync sucess. Error code: {0}.") << it->key_;
									}
								}
								else
								{
									QLOG_ERR(L"QueueListAsync error. Error code: {0}.") << error_code;
								}
							};
							ChatRoom::QueueListAsync(room_id, cb);
#endif
						}
						default:
							break;
						}

					}
				}
			}
			return;
		}

	}

	void ChatroomCallback::OnReceiveSysmsgCallback(const nim::SysMessage& msg)
	{
		QLOG_PRO(L"OnReceiveSysmsgCallback: content: {0} - attach : {1}") << msg.content_ << msg.attach_;
		StdClosure task = nbase::Bind(&UIReceiveSysmsgCallback, msg);
		Post2UI(task);
	}

void ChatroomCallback::OnReceiveMsgCallback(__int64 room_id, const ChatRoomMessage& result)
{
	QLOG_PRO(L"Chatroom:OnReceiveMsgCallback: {0}") << result.client_msg_id_;
	//接收自定义消息，更改旁路直播状态信息 

	StdClosure cb = [=](){
		ChatroomForm* chat_form = static_cast<ChatroomForm*>(nim_ui::WindowsManager::GetInstance()->GetWindow(ChatroomForm::kClassName, nbase::Int64ToString16(room_id)));
		if (chat_form != NULL && result.msg_type_ != kNIMChatRoomMsgTypeUnknown)
		{
			chat_form->OnReceiveMsgCallback(result);
		}
	};
	Post2UI(cb);
}

void ChatroomCallback::OnSendMsgCallback(__int64 room_id, int error_code, const ChatRoomMessage& result)
{
	QLOG_APP(L"Chatroom:OnSendMsgCallback: id={0} msg_id={1} code={2}") << result.room_id_ << result.client_msg_id_ << error_code;
}

void ChatroomCallback::OnEnterCallback(__int64 room_id, const NIMChatRoomEnterStep step, int error_code, const ChatRoomInfo& info, const ChatRoomMemberInfo& my_info)
{
	QLOG_APP(L"Chatroom:OnEnterCallback: id={0} step={1} code={2}") << room_id << step << error_code;

	if (step != kNIMChatRoomEnterStepRoomAuthOver)
		return;

	StdClosure cb = [=](){
		if (error_code != nim::kNIMResSuccess && error_code != nim::kNIMResTimeoutError)
		{
			ChatroomForm* chat_form = static_cast<ChatroomForm*>(nim_ui::WindowsManager::GetInstance()->GetWindow(ChatroomForm::kClassName, nbase::Int64ToString16(room_id)));
			if (chat_form)
			{
				if (error_code == nim::kNIMResRoomLocalNeedRequestAgain)
				{//重新登录
					chat_form->RequestEnter(room_id);
					return;
				}
				else
					chat_form->Close();
			}
		}
		else
		{
			ChatroomForm* chat_form = static_cast<ChatroomForm*>(nim_ui::WindowsManager::GetInstance()->GetWindow(ChatroomForm::kClassName, nbase::Int64ToString16(room_id)));
			if (chat_form != NULL)
			chat_form->OnEnterCallback(error_code, info, my_info);
		}
	};
	Post2UI(cb);
}

void ChatroomCallback::OnExitCallback(__int64 room_id, int error_code, NIMChatRoomExitReason exit_reason)
{
	QLOG_APP(L"Chatroom:OnExitCallback: id={0} code={1}") << room_id << error_code;

	StdClosure cb = [room_id, exit_reason]()
	{
		ChatroomForm* chat_form = static_cast<ChatroomForm*>(nim_ui::WindowsManager::GetInstance()->GetWindow(ChatroomForm::kClassName, nbase::Int64ToString16(room_id)));
		if (chat_form)
			chat_form->Close(ChatroomForm::kAllowClose);

		if (exit_reason == kNIMChatRoomExitReasonExit)
			return;
	};
	Post2UI(cb);
}

void ChatroomCallback::OnNotificationCallback(__int64 room_id, const ChatRoomNotification& notification)
{
	QLOG_APP(L"Chatroom:OnNotificationCallback: id={0}") << room_id;
	//QLOG_APP(L"Chatroom:OnNotificationCallback: ext : {0}") << notification.ext_;

	StdClosure cb = [=](){
		ChatroomForm* chat_form = static_cast<ChatroomForm*>(nim_ui::WindowsManager::GetInstance()->GetWindow(ChatroomForm::kClassName, nbase::Int64ToString16(room_id)));
		if (chat_form != NULL)
		{
			chat_form->OnNotificationCallback(notification);
		}
	};
	Post2UI(cb);
}

void ChatroomCallback::OnRegLinkConditionCallback(__int64 room_id, const NIMChatRoomLinkCondition condition)
{
	QLOG_APP(L"Chatroom:OnRegLinkConditionCallback: id={0} condition={1}") << room_id << condition;

	StdClosure cb = [=](){
		ChatroomForm* chat_form = static_cast<ChatroomForm*>(nim_ui::WindowsManager::GetInstance()->GetWindow(ChatroomForm::kClassName, nbase::Int64ToString16(room_id)));
		if (chat_form != NULL)
		{
			chat_form->OnRegLinkConditionCallback(room_id, condition);
		}
	};
	Post2UI(cb);

}

void ChatroomCallback::InitChatroomCallback()
{
	nim::SystemMsg::RegSysmsgCb(nbase::Bind(&ChatroomCallback::OnReceiveSysmsgCallback, std::placeholders::_1));

	ChatRoom::RegReceiveMsgCb(nbase::Bind(&ChatroomCallback::OnReceiveMsgCallback, std::placeholders::_1, std::placeholders::_2));
	ChatRoom::RegSendMsgAckCb(nbase::Bind(&ChatroomCallback::OnSendMsgCallback, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	ChatRoom::RegEnterCb(nbase::Bind(&ChatroomCallback::OnEnterCallback, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
	ChatRoom::RegExitCb(nbase::Bind(&ChatroomCallback::OnExitCallback, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	ChatRoom::RegNotificationCb(nbase::Bind(&ChatroomCallback::OnNotificationCallback, std::placeholders::_1, std::placeholders::_2));
	ChatRoom::RegLinkConditionCb(nbase::Bind(&ChatroomCallback::OnRegLinkConditionCallback, std::placeholders::_1, std::placeholders::_2));

	//将旁路通知的系统通知
}


}