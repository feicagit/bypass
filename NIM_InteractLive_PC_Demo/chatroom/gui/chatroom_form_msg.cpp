#include "chatroom_form.h"
#include "player/player_manager.h"

#define ROOMMSG_R_N _T("\r\n")

namespace nim_chatroom
{
	using namespace ui;
	using namespace nim_comp;

	void ChatroomForm::RequestEnter(const __int64 room_id)
	{
		if (room_id == 0)
		{
			RequestRoomError();
			return;
		}

		room_id_ = room_id;

		Json::FastWriter fw;
		Json::Value value;
		value["type"] = living_model_;
		value["meetingName"] = room_name_;
		std::string json_value = fw.write(value);
		//获取聊天室登录信息
		nim::PluginIn::ChatRoomRequestEnterAsync(room_id_, nbase::Bind(&ChatroomForm::OnChatRoomRequestEnterCallback, this, std::placeholders::_1, std::placeholders::_2), json_value);
	}

	__int64 ChatroomForm::GetRoomId()
	{
		return room_id_;
	}
	std::string ChatroomForm::GetRoomSessionId()
	{
		std::string session_id = nbase::StringPrintf(UINT64_FORMAT, GetRoomId());
		return session_id;
	}

	std::string ChatroomForm::GetRoomName()
	{
		return room_name_;
	}

	void ChatroomForm::OnReceiveMsgCallback(const ChatRoomMessage& result)
	{
		//{"data":{"opeNick":"123","operator":"leewp","tarNick":["123"],"target":["leewp"]},"id":301}
		if (result.msg_type_ == kNIMChatRoomMsgTypeNotification)
		{
			// 登录成功后，会收到一个“欢迎进入直播间”的通知消息，这个消息的时间戳作为获取历史消息的标准
			// 通知消息由OnNotificationCallback回调处理，这里不做处理
			if (time_start_history == 0)
				time_start_history = result.timetag_ - 1;
			if (time_start_history < 0)
				time_start_history = 0;

			Json::Reader reader;
			Json::Value value;
			if (reader.parse(result.msg_attach_, value))
			{
				if (value.isMember("meetingName"))
				{
					string sessionid = value["meetingName"].asString();
					SetChatRoomRoomName(sessionid);
				}
			}
			return;
		}

		if (result.msg_type_ == kNIMChatRoomMsgTypeCustom)
		{
			Json::Reader reader;
			Json::Value  bypassinfo;
			int bypassopt;
			string nick;
			int  style;
			string uid;
			int count;
			int present;
			if (reader.parse(result.msg_attach_, bypassinfo))
			{
				//石头剪刀布的消息格式 {"data":{"value":3}, "type" : 2}

				//{"type":0, "data" : {"present":0, "count" : 1}}
				bypassopt = bypassinfo["type"].asInt();//同意，还是取消
				nick = bypassinfo["data"]["nick"].asString();
				style = bypassinfo["data"]["style"].asInt();
				uid = bypassinfo["data"]["uid"].asString();
				count = bypassinfo["data"]["count"].asInt();
				present = bypassinfo["data"]["present"].asInt();
				if (uid.empty() || uid == "0")
				{
					uid = LoginManager::GetInstance()->GetAccount();
					StdClosure closure = [=]()
					{
						if (master_)
						{
							if (bypassopt == 0)
							{
								AddGiftNumbers((GiftType)present, count);
								AddGiftInfo(result.from_id_, (GiftType)present, count);
							}
							else if (bypassopt == 1)
							{
								AddGiftInfo(result.from_id_, GiftType::kGiftRedHeart, 1);
							}
						}

						if (uid == creater_id_)
							return;
						//收到自定义消息
						//观众收到自定义消息
						//连麦者离开音频通道
						//更新旁路互动信息
						string 	nick = nbase::UTF16ToUTF8(nim_ui::UserManager::GetInstance()->GetUserName(uid, false));
						if (style != 0)
						{
							switch (bypassopt)
							{
							case NTESAnchorAgree:

							{
								UpdateBypassMembersState(uid, NTESLiveMicStateConnected, (InactionType)style, kAdd, nick);
							}
								break;
							case kNTESAnchorRejectOrDisConnect:
								//连麦者离开音频通道 
							{
								UpdateBypassMembersState(uid, NTESLiveMicStateConnected, (InactionType)bypassopt, kRemove, nick);
							}
								break;
							default:
								break;
							}
						}

					};
					Post2UI(closure);
				}
				else
				{
					StdClosure closure = [=](){
						if (uid == creater_id_)
							return;
						string 	nick = nbase::UTF16ToUTF8(nim_ui::UserManager::GetInstance()->GetUserName(uid, false));
						switch (bypassopt)
						{
						case NTESAnchorAgree:
						{
							UpdateBypassMembersState(uid, NTESLiveMicStateConnected, (InactionType)style, kAdd, nick);
						}
							break;
						case kNTESAnchorRejectOrDisConnect:
						{
							UpdateBypassMembersState(uid, NTESLiveMicStateConnected, (InactionType)bypassopt, kRemove, nick);
						}
						break;
						default:
							break;
						}
					};
					Post2UI(closure);
				}
			}
			else
			{
				return;
			}
		}
		AddMsgItem(result, false);
	}

	void ChatroomForm::OnEnterCallback(int error_code, const ChatRoomInfo& info, const ChatRoomMemberInfo& my_info)
	{

		if (error_code == nim::kNIMResTimeoutError)
		{
			//底层会尝试重连
			return;
		}
		if (error_code != nim::kNIMResSuccess)
		{
			Close();
			return;
		}
		if (info.id_ == 0)
		{
			return;
		}

		room_id_ = info.id_;
		has_enter_ = true;

		StdClosure task = [=](){
			string json_living_model = info.ext_;
			Json::Reader reader;
			Json::Value value;
			int living_status_ = 0;
			if (reader.parse(json_living_model, value))
			{
				living_status_ = value["type"].asInt();
				room_name_ = value["meetingName"].asString();
			}
			std::wstring name = my_info.nick_.empty() ? nim_ui::UserManager::GetInstance()->GetUserName(my_info.account_id_, false) : nbase::UTF8ToUTF16(my_info.nick_);
			name_->SetText(name);

			ASSERT(!info.creator_id_.empty());
			creater_id_ = info.creator_id_;
			CheckMaster();

			if (!master_)
			{
				btn_start_ls_->SetVisible(false);
				ls_time_->SetVisible(false);
				btn_interact_set_->SetVisible(true);
				btn_audiohook_set->SetVisible(false);
				option_bypass_gift_->SetVisible(false);
				SetLivingModel((InactionType)living_status_);
				//StdClosure task = nbase::Bind(&ChatroomForm::CheckIPState, this);
				//nbase::ThreadManager::PostRepeatedTask(kThreadLiveStreaming, check_ip_status_timer_.ToWeakCallback(task), nbase::TimeDelta::FromMilliseconds(5000));//5s检测一次

			}
			else
			{
				if (audience_)
				{
					auto msg_cb = [this](MsgBoxRet ret)
					{
						//ChatRoom::Exit(room_id_);
						Close();
					};
					ShowMsgBox(m_hWnd, L"主播不能以观众身份进入自己的直播间", msg_cb, L"提示", L"确定", L"");
					return;
				}
				else
				{
					btn_interact_set_->SetVisible(false);
					//预览
					OnLiveStreamInit(true);
				}
			}
			//RequestAddress(nbase::IntToString(room_id_), LoginManager::GetInstance()->GetAccount());
			InitChatRoomQueueInfo();

		};
		Post2UI(task);

		OnGetChatRoomInfoCallback(room_id_, error_code, info);
		GetMembers();
	}

	void ChatroomForm::OnGetChatRoomInfoCallback(__int64 room_id, int error_code, const ChatRoomInfo& info)
	{
		if (error_code != nim::kNIMResSuccess || room_id != room_id_)
		{
			return;
		}

		StdClosure cb = [=](){
			time_refresh_ = time(NULL);

			if (!info.announcement_.empty())
			{
				
				//bulletin_->SetUTF8Text(info.announcement_);
				bulletin_->SetBkImage(L"");
				if (nim_ui::LoginManager::GetInstance()->GetAccount() == info.creator_id_)
				{
					bulletin_->SetReadOnly(false);
				}
			}
			std::wstring room_name = nbase::StringPrintf(L"%s[直播%lld]", nbase::UTF8ToUTF16(info.name_).c_str(), room_id_);
			name_->SetText(room_name);
		};
		Post2UI(cb);
	}

	void ChatroomForm::OnNotificationCallback(const ChatRoomNotification& notification)
	{
		AddNotifyItem(notification, false);
	}

	void ChatroomForm::GetMembers()
	{
		ChatRoomGetMembersParameters member_param;
		member_param.type_ = kNIMChatRoomGetMemberTypeSolid;
		ChatRoom::GetMembersOnlineAsync(room_id_, member_param, nbase::Bind(&ChatroomForm::OnGetMembersCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

		member_param.type_ = kNIMChatRoomGetMemberTypeTemp;
		ChatRoom::GetMembersOnlineAsync(room_id_, member_param, nbase::Bind(&ChatroomForm::OnGetMembersCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	}

	void ChatroomForm::OnGetMembersCallback(__int64 room_id, int error_code, const std::list<ChatRoomMemberInfo>& infos)
	{
		if (error_code != nim::kNIMResSuccess || room_id_ != room_id)
			return;

		StdClosure cb = [=](){
			empty_members_list_->SetVisible(false);

			for (auto& it : infos)
			{
				members_list_[it.account_id_] = it;
				nick_account_map_[it.nick_] = it.account_id_;

				// 不加入离线的游客
				if (it.guest_flag_ == kNIMChatRoomGuestFlagGuest && it.state_ == kNIMChatRoomOnlineStateOffline)
					continue;

				if (NULL != online_members_list_->FindSubControl(nbase::UTF8ToUTF16(it.account_id_)))
					continue;

				ui::ButtonBox* room_member_item = (ui::ButtonBox*)ui::GlobalManager::CreateBoxWithCache(L"chatroom/room_member_item.xml");
				if (room_member_item)
				{
					ui::Control* member_type = (ui::Control*)room_member_item->FindSubControl(L"member_type");
					ui::Control* header_image = (ui::Control*)room_member_item->FindSubControl(L"header_image");
					ui::Label* name = (ui::Label*)room_member_item->FindSubControl(L"name");

					if (it.type_ == kCreater)//创建者++++++++++++
					{
						member_type->SetBkImage(L"icon_anchor.png");
						online_members_list_->AddAt(room_member_item, 0);
						has_add_creater_ = true;
					}
					else if (it.type_ == kManager)//管理员  
					{
						member_type->SetBkImage(L"icon_manager.png");
						managers_list_[it.account_id_] = it;
						if (online_members_list_->GetCount() == 0)
						{
							online_members_list_->Add(room_member_item);
						}
						else
						{
							online_members_list_->AddAt(room_member_item, 1);
						}
					}
					else
					{
						member_type->SetVisible(false);
						online_members_list_->Add(room_member_item);
					}

					name->SetUTF8Text(it.nick_);
					if (!it.avatar_.empty() && it.avatar_.find_first_of("http") == 0)
						header_image->SetBkImage(nim_ui::HttpManager::GetInstance()->GetCustomImage(kChatroomMemberIcon, it.account_id_, it.avatar_));
					else
						header_image->SetBkImage(nim_ui::PhotoManager::GetInstance()->GetUserPhoto(it.account_id_));

					room_member_item->SetUTF8Name(it.account_id_);
				}
			}
		};
		Post2UI(cb);
	}

	void ChatroomForm::OnChatRoomRequestEnterCallback(int error_code, const std::string& result)
	{
		StdClosure closure_err = ToWeakCallback([this, error_code]()
		{
			RequestRoomError();
		});

		if (error_code != nim::kNIMResSuccess)
		{
			Post2UI(closure_err);
			QLOG_APP(L"OnChatRoomRequestEnterCallback error: error id={0}") << error_code;
			return;
		}

		if (!result.empty())
		{
			StdClosure cb = [result, this](){
				room_enter_token_ = result;
				ChatRoomEnterInfo info;
				Json::FastWriter fw;
				Json::Value value;
				value["meetingName"] = room_name_;
				if (NeedNotifyExt())
					info.SetNotifyExt(value);
				bool bRet = ChatRoom::Enter(room_id_, room_enter_token_, info);
				if (bRet)
				{
					this->CenterWindow();
					this->ShowWindow();
				}
				else
				{
					QLOG_APP(L"ChatRoom::Enter error");
					RequestRoomError();
				}
			};
			Post2UI(cb);
		}
		else
		{
			Post2UI(closure_err);
		}
	}

	void ChatroomForm::OnRegLinkConditionCallback(__int64 room_id, const NIMChatRoomLinkCondition condition)
	{
		if (room_id_ != room_id)
			return;
		chat_room_link_condition_ = condition;
		if (condition == kNIMChatRoomLinkConditionAlive && has_enter_)
		{
			
		}
		else if (condition == kNIMChatRoomLinkConditionDead)
		{
			msg_list_->SetText(L"");
			input_edit_->SetText(L"");
		}
	}

	void ChatroomForm::AddMsgItem(const ChatRoomMessage& result, bool is_history)
	{
		if (result.msg_type_ == kNIMChatRoomMsgTypeText)
		{
			AddText(nbase::UTF8ToUTF16(result.msg_attach_), nbase::UTF8ToUTF16(result.from_nick_), is_history);
		}
		else if (result.msg_type_ == kNIMChatRoomMsgTypeNotification)
		{
			Json::Value json;
			Json::Reader reader;
			if (reader.parse(result.msg_attach_, json))
			{
				ChatRoomNotification notification;
				notification.ParseFromJsonValue(json);
				AddNotifyItem(notification, is_history);
			}
		}
		else if (result.msg_type_ == kNIMChatRoomMsgTypeCustom)
		{
			Json::Value json;
			Json::Reader reader;
			if (reader.parse(result.msg_attach_, json))
			{
				int sub_type = json["type"].asInt();
				if (sub_type == CustomMsgType_Jsb && json["data"].isObject())
				{
					int value = json["data"]["value"].asInt();
					AddJsb(value, nbase::UTF8ToUTF16(result.from_nick_), is_history);
				}
			}
		}

		if (!result.from_nick_.empty())
			nick_account_map_[result.from_nick_] = result.from_id_;
	}

	void ChatroomForm::AddNotifyItem(const ChatRoomNotification& notification, bool is_history)
	{
		std::string my_id = nim_ui::LoginManager::GetInstance()->GetAccount();

		if (notification.id_ == kNIMChatRoomNotificationIdInfoUpdated)
		{
			Json::Reader reader;
			Json::Value value;
			if (reader.parse(notification.ext_, value))
			{
				room_name_ = value["meetingName"].asString();
				int statue = value["type"].asInt();
			std::wstring prompt_info = L"";
				//主播结束直播
				if (statue == kUnLive&&!master_)
				{
					QLOG_ERR(L"AddNotifyItem: the living end");
					prompt_info = L"直播已结束";
					StdClosure closure = [this, prompt_info]()
					{
						auto box_cb = [this](MsgBoxRet)
						{
							//ChatRoom::Exit(room_id_);
							Close();
						};
						//check_chatroom_status_timer_.Cancel();
						ShowMsgBox(m_hWnd, prompt_info, box_cb, L"提示", L"确定", L"");
					};
					Post2UI(closure);
				}
			}
		}

		auto it_nick = notification.target_nick_.cbegin();
		auto it_id = notification.target_ids_.cbegin();
		for (;	it_nick != notification.target_nick_.cend(), it_id != notification.target_ids_.cend();
			++it_id, ++it_nick)
		{
			std::wstring nick = nbase::UTF8ToUTF16(*it_nick);
			if (*it_id == my_id)
				nick = L"你";

			std::wstring str;
			if (notification.id_ == kNIMChatRoomNotificationIdMemberIn)
			{
				if (!master_)
				{
					Json::Reader reader;
					Json::Value value;
					if (reader.parse(notification.ext_, value))
					{
						room_name_ = value["meetingName"].asString();
					}
				}
				str = nbase::StringPrintf(L"欢迎%s进入直播间", nick.c_str());
			}
			else if (notification.id_ == kNIMChatRoomNotificationIdMemberExit)
			{
				str = nbase::StringPrintf(L"%s退出直播间", nick.c_str());
				if (*it_id == creater_id_)
				{
					//auto cb = [this](MsgBoxRet)
					//{
						//ChatRoom::Exit(room_id_);
					//	ChatRoom::QueueDropAsync(room_id_, nullptr);
					//	Close();
					//};

					//ShowMsgBox(m_hWnd, L"直播已结束", cb, L"提示", L"确定", L"");
				}
				else
				{
					ChatRoom::QueuePollAsync(room_id_, *it_id, nullptr);
					UpdateBypassMembersState(*it_id, NTESLiveMicStateConnected, kVedio, kRemove, *it_nick);
				}

				RemoveMember(*it_id);
			}

			AddNotify(str, is_history);
		}
	}

	void ChatroomForm::OnBtnSend()
	{
		if (!master_&&!is_pull_normal_)
			return;

		if (chat_room_link_condition_ != kNIMChatRoomLinkConditionAlive)
		{
			msg_list_->SetText(L"");
			return;
		}

		auto my_info = members_list_.find(nim_ui::LoginManager::GetInstance()->GetAccount());
		if (my_info != members_list_.end() && !my_info->second.is_muted_)
		{
			wstring sText;
			nim_comp::Re_GetText(input_edit_->GetTextServices(), sText);
			StringHelper::Trim(sText);
			if (sText.empty()) return;
			input_edit_->SetText(_T(""));
			SendText(nbase::UTF16ToUTF8(sText));

			std::wstring my_name = nim_ui::UserManager::GetInstance()->GetUserName(nim_ui::LoginManager::GetInstance()->GetAccount(), false);
			AddText(sText, my_name, false);
		}
	}

	void ChatroomForm::AddText(const std::wstring &text, const std::wstring &sender_name, bool is_history)
	{
		if (text.empty()) return;
		// 
		if (!is_history)
		{
			//video_barrage_ctrl_->AddText(text);
		}
		if (msg_list_->GetTextLength() != 0)
		{
			if (is_history)
			{
				msg_list_->SetSel(0, 0);
				msg_list_->ReplaceSel(ROOMMSG_R_N, false);
			}
			else
			{
				msg_list_->SetSel(-1, -1);
				msg_list_->ReplaceSel(ROOMMSG_R_N, false);
			}
		}

		//
		long lSelBegin = 0, lSelEnd = 0;
		CHARFORMAT2 cf;
		msg_list_->GetDefaultCharFormat(cf); //获取消息字体
		cf.dwMask = CFM_LINK | CFM_FACE | CFM_COLOR;
		cf.dwEffects = CFE_LINK;

		// 添加发言人，并设置他的颜色
		lSelEnd = lSelBegin = is_history ? 0 : msg_list_->GetTextLength();
		msg_list_->SetSel(lSelEnd, lSelEnd);
		msg_list_->ReplaceSel(sender_name, false);

		lSelEnd = is_history ? sender_name.length() : msg_list_->GetTextLength();
		msg_list_->SetSel(lSelBegin, lSelEnd);
		msg_list_->SetSelectionCharFormat(cf);

		// 设置文本的缩进
		PARAFORMAT2 pf;
		ZeroMemory(&pf, sizeof(PARAFORMAT2));
		pf.cbSize = sizeof(pf);
		pf.dwMask = PFM_STARTINDENT | PFM_LINESPACING;
		LONG lineSpace = (LONG)(1.2 * 20);//设置1.2行间距
		pf.dxStartIndent = 0;
		pf.bLineSpacingRule = 5;
		pf.dyLineSpacing = lineSpace;
		msg_list_->SetParaFormat(pf);
		msg_list_->SetSel(lSelEnd, lSelEnd);
		msg_list_->ReplaceSel(ROOMMSG_R_N, false);
		lSelEnd++;
		// 添加正文，并设置他的颜色	
		msg_list_->SetSel(lSelEnd, lSelEnd);
		nim_comp::emoji::InsertToEdit(msg_list_, text);
		//设置文本字体
		msg_list_->GetDefaultCharFormat(cf); //获取消息字体
		long lSelBegin2 = 0, lSelEnd2 = 0;
		msg_list_->GetSel(lSelBegin2, lSelEnd2);
		msg_list_->SetSel(lSelEnd, lSelEnd2);
		msg_list_->SetSelectionCharFormat(cf);

		// 设置正文文本的缩进
		msg_list_->SetSel(lSelEnd, lSelEnd);
		pf.dxStartIndent = 150;
		msg_list_->SetParaFormat(pf);

		if (!is_history)
			msg_list_->EndDown();
		input_edit_->SetFocus();
	}

	void ChatroomForm::AddGiftInfo(const std::string& id, GiftType type, int number)
	{
		std::wstring nick = nim_ui::UserManager::GetInstance()->GetUserName(id, false);
		std::wstring notify = L"送给您 ";
		std::wstring str_nummber = nbase::IntToString16(number);
		std::wstring file_name = L"";
		std::wstring  classifier = L"个";
		if (notify.empty()) return;

		if (msg_list_->GetTextLength() != 0)
		{
			msg_list_->SetSel(-1, -1);
			msg_list_->ReplaceSel(ROOMMSG_R_N, false);
		}

		switch (type)
		{
		case nim_chatroom::kUnKnown:
			break;
		case nim_chatroom::kGiftRose:
			file_name = L"rose_small.png";
			classifier = L"束";
			break;
		case nim_chatroom::kGiftChocolate:
			file_name = L"cholocate_small.png";
			break;
		case nim_chatroom::kGiftBear:
			file_name = L"bear_small.png";
			classifier = L"只";
			break;
		case nim_chatroom::kGiftIceCream:
			file_name = L"ice_small.png";
			break;
		case nim_chatroom::kGiftRedHeart:
			file_name = L"red_heart.png";
			classifier = L"个";
			break;
		default:
			break;
		}
		// 
		long lSelBegin = 0, lSelEnd = 0;
		CHARFORMAT2 cf;
		msg_list_->GetDefaultCharFormat(cf); //获取消息字体
		cf.dwMask |= CFM_COLOR;
		cf.crTextColor = RGB(240, 91, 96);

		//昵称
		lSelEnd = lSelBegin = msg_list_->GetTextLength();
		msg_list_->SetSel(lSelEnd, lSelEnd);
		msg_list_->ReplaceSel(nick, false);
		lSelEnd += nick.length();
		msg_list_->SetSel(lSelBegin, lSelEnd);
		msg_list_->SetSelectionCharFormat(cf);

		// 添加通知消息，并设置他的颜色
		lSelEnd = lSelBegin = msg_list_->GetTextLength();
		msg_list_->SetSel(lSelEnd, lSelEnd);
		msg_list_->ReplaceSel(notify, false);
		lSelEnd += notify.length();
		msg_list_->GetDefaultCharFormat(cf);
		cf.dwMask |= CFM_COLOR;
		cf.crTextColor = RGB(0, 0, 0);
		msg_list_->SetSel(lSelBegin, lSelEnd);
		msg_list_->SetSelectionCharFormat(cf);

		//添加图片
		msg_list_->SetSel(lSelEnd, lSelEnd);
		if (file_name.empty()) return;
		std::wstring file = QPath::GetAppPath();
		file.append(L"themes\\default\\chatroom\\" + file_name);
		if (nbase::FilePathIsExist(file, false))
		{
			ITextServices* service = msg_list_->GetTextServices();
			if (!nim_comp::Re_InsertJsb(service, file, file_name))
			{
				QLOG_ERR(L"insert gift image {0} fail") << file;
			}
			service->Release();
		}
		else
		{
			QLOG_ERR(L"gift image {0} miss") << file;
		}
		lSelEnd++;
		//添加礼物数
		lSelEnd = lSelBegin = msg_list_->GetTextLength();
		msg_list_->SetSel(lSelEnd, lSelEnd);
		msg_list_->ReplaceSel(str_nummber, false);
		lSelEnd += str_nummber.length();

		msg_list_->GetDefaultCharFormat(cf);
		cf.dwMask |= CFM_COLOR;
		cf.crTextColor = RGB(240, 91, 96);
		msg_list_->SetSel(lSelBegin, lSelEnd);
		msg_list_->SetSelectionCharFormat(cf);
		msg_list_->SetSel(lSelBegin, lSelEnd);
		msg_list_->SetSelectionCharFormat(cf);

		//添加量词
		lSelEnd = lSelBegin = msg_list_->GetTextLength();
		msg_list_->SetSel(lSelEnd, lSelEnd);
		msg_list_->ReplaceSel(classifier, false);
		lSelEnd += classifier.length();
		msg_list_->GetDefaultCharFormat(cf);
		cf.dwMask |= CFM_COLOR;
		cf.crTextColor = RGB(240, 91, 96);
		msg_list_->SetSel(lSelBegin, lSelEnd);
		msg_list_->SetSelectionCharFormat(cf);
		msg_list_->SetSel(lSelBegin, lSelEnd);
		msg_list_->SetSelectionCharFormat(cf);



		// 设置正文文本的缩进
		msg_list_->SetSel(lSelEnd, lSelEnd);
		PARAFORMAT2 pf;
		ZeroMemory(&pf, sizeof(PARAFORMAT2));
		pf.cbSize = sizeof(pf);
		pf.dwMask = PFM_STARTINDENT;
		pf.dxStartIndent = 150;
		msg_list_->SetParaFormat(pf);

		msg_list_->EndDown();
	}

	void ChatroomForm::AddGiftNumbers(GiftType type, int number)
	{
		gift_prompt_->SetVisible(false);
		std::wstring str = L"";
		auto it = gift_map_.find(type);
		GiftInfo* gift_control_info_ = NULL;
		if (it == gift_map_.end())
		{
			gift_control_info_ = new GiftInfo();
			int i = gift_map_.size();
			switch (i)
			{
			case 0:
				gift_box_1_->SetVisible(true);
				gift_control_info_->pImage = gift_png_1_;
				gift_control_info_->pInfo = lb_gift_1_;
				gift_control_info_->pCount = lb_gift_1_count_;
				break;
			case 1:
				gift_box_2_->SetVisible(true);
				gift_control_info_->pImage = gift_png_2_;
				gift_control_info_->pInfo = lb_gift_2_;
				gift_control_info_->pCount = lb_gift_2_count_;
				break;
			case 2:
				gift_box_3_->SetVisible(true);
				gift_control_info_->pImage = gift_png_3_;
				gift_control_info_->pInfo = lb_gift_3_;
				gift_control_info_->pCount = lb_gift_3_count_;
				break;
			case 3:
				gift_box_4_->SetVisible(true);
				gift_control_info_->pImage = gift_png_4_;
				gift_control_info_->pInfo = lb_gift_4_;
				gift_control_info_->pCount = lb_gift_4_count_;
				break;
			default:
				break;
			}
			gift_map_[type] = *gift_control_info_;
		}
		else
		{
			gift_control_info_ = &it->second;
		}
		gift_control_info_->pImage->SetVisible(true);
		gift_control_info_->pInfo->SetVisible(true);
		gift_control_info_->pCount->SetVisible(true);
		switch (type)
		{
		case nim_chatroom::kGiftRose:
			gift_rose_count_ += number;
			str = L"玫瑰x";
			gift_control_info_->pImage->SetFixedHeight(60);
			gift_control_info_->pImage->SetFixedWidth(52);
			gift_control_info_->pImage->SetBkImage(L"rose_1.png");
			gift_control_info_->pInfo->SetText(str);
			gift_control_info_->pCount->SetUTF8Text(nbase::IntToString(gift_rose_count_));
			break;
		case nim_chatroom::kGiftChocolate:
			gift_cholocate_count_ += number;
			str = L"巧克力x";
			gift_control_info_->pImage->SetFixedHeight(60);
			gift_control_info_->pImage->SetFixedWidth(100);
			gift_control_info_->pImage->SetBkImage(L"cholocate_1.png");
			gift_control_info_->pInfo->SetText(str);
			gift_control_info_->pCount->SetUTF8Text(nbase::IntToString(gift_cholocate_count_));
			break;
		case nim_chatroom::kGiftBear:
			gift_bear_count_ += number;
			str = L"熊x";
			gift_control_info_->pImage->SetFixedHeight(60);
			gift_control_info_->pImage->SetFixedWidth(52);
			gift_control_info_->pImage->SetBkImage(L"bear_1.png");
			gift_control_info_->pInfo->SetText(str);
			gift_control_info_->pCount->SetUTF8Text(nbase::IntToString(gift_bear_count_));
			break;
		case nim_chatroom::kGiftIceCream:
			gift_ice_count_ += number;
			str = L"冰激凌x";
			gift_control_info_->pImage->SetFixedHeight(60);
			gift_control_info_->pImage->SetFixedWidth(33);
			gift_control_info_->pImage->SetBkImage(L"ice_1.png");
			gift_control_info_->pInfo->SetText(str);
			gift_control_info_->pCount->SetUTF8Text(nbase::IntToString(gift_ice_count_));
			break;
		default:
			break;
		}
	}

	void ChatroomForm::AddNotify(const std::wstring &notify, bool is_history)
	{
		if (notify.empty()) return;
		// 
		if (msg_list_->GetTextLength() != 0)
		{
			if (is_history)
			{
				msg_list_->SetSel(0, 0);
				msg_list_->ReplaceSel(ROOMMSG_R_N, false);
			}
			else
			{
				msg_list_->SetSel(-1, -1);
				msg_list_->ReplaceSel(ROOMMSG_R_N, false);
			}
		}

		//
		long lSelBegin = 0, lSelEnd = 0;
		CHARFORMAT2 cf;
		msg_list_->GetDefaultCharFormat(cf); //获取消息字体
		cf.dwMask |= CFM_COLOR;
		cf.crTextColor = RGB(255, 0, 0);

		// 添加通知消息，并设置他的颜色
		lSelEnd = lSelBegin = is_history ? 0 : msg_list_->GetTextLength();
		msg_list_->SetSel(lSelEnd, lSelEnd);
		msg_list_->ReplaceSel(notify, false);

		lSelEnd = is_history ? notify.length() : msg_list_->GetTextLength();
		msg_list_->SetSel(lSelBegin, lSelEnd);
		msg_list_->SetSelectionCharFormat(cf);

		// 设置正文文本的缩进
		msg_list_->SetSel(lSelEnd, lSelEnd);
		PARAFORMAT2 pf;
		ZeroMemory(&pf, sizeof(PARAFORMAT2));
		pf.cbSize = sizeof(pf);
		pf.dwMask = PFM_STARTINDENT;
		pf.dxStartIndent = 150;
		msg_list_->SetParaFormat(pf);

		if (!is_history)
			msg_list_->EndDown();
	}

	void ChatroomForm::SendImage(const std::wstring &src)
	{
		nim::IMImage img;
		std::string utf8 = nbase::UTF16ToUTF8(src);
		img.md5_ = nim::Tool::GetFileMd5(utf8);
		img.size_ = (long)nbase::GetFileSize(src);
		std::wstring file_name, file_ext;
		nbase::FilePathApartFileName(src, file_name);
		nbase::FilePathExtension(src, file_ext);
		img.display_name_ = nbase::UTF16ToUTF8(file_name);
		img.file_extension_ = nbase::UTF16ToUTF8(file_ext);
		Gdiplus::Image image(src.c_str());
		if (image.GetLastStatus() != Gdiplus::Ok)
		{
			assert(0);
		}
		else
		{
			img.width_ = image.GetWidth();
			img.height_ = image.GetHeight();
		}
		auto weak_flag = this->GetWeakFlag();
		nim::NOS::UploadResource(utf8, [this, img, weak_flag](nim::NIMResCode res_code, const std::string& url) {
			if (!weak_flag.expired() && res_code == nim::kNIMResSuccess)
			{
				nim_chatroom::ChatRoomMessageSetting setting;
				std::string send_msg = ChatRoom::CreateRoomMessage(kNIMChatRoomMsgTypeImage, QString::GetGUID(), img.ToJsonString(), setting);
				ChatRoom::SendMsg(room_id_, send_msg);
			}
		});
	}

	void ChatroomForm::OnUploadImageCallback(nim::NIMResCode res_code, const std::string& url, nim::IMImage img)
	{
		nim_chatroom::ChatRoomMessageSetting setting;
		std::string send_msg = ChatRoom::CreateRoomMessage(kNIMChatRoomMsgTypeImage, QString::GetGUID(), img.ToJsonString(), setting);
		ChatRoom::SendMsg(room_id_, send_msg);
	}

	void ChatroomForm::SendText(const std::string &text)
	{
		//std::string test_string = "{\"remote\":{\"mapmap\":{\"int\":1,\"boolean\":false,\"list\":[1,2,3],\"string\":\"string, lalala\"}}}";
		nim_chatroom::ChatRoomMessageSetting setting;
		std::string send_msg = ChatRoom::CreateRoomMessage(kNIMChatRoomMsgTypeText, QString::GetGUID(), text, setting);
		ChatRoom::SendMsg(room_id_, send_msg);
	}

	void ChatroomForm::OnBtnJsb()
	{
		if (!master_&&!is_pull_normal_)
			return;
		if (chat_room_link_condition_!=kNIMChatRoomLinkConditionAlive)
		{
			msg_list_->SetText(L"");
			return;
		}

		auto my_info = members_list_.find(nim_ui::LoginManager::GetInstance()->GetAccount());
		if (my_info != members_list_.end() && !my_info->second.is_muted_)
		{
			int jsb = (rand() % 3 + rand() % 4 + rand() % 5) % 3 + 1;

			Json::Value json;
			Json::FastWriter writer;
			json["type"] = CustomMsgType_Jsb;
			json["data"]["value"] = jsb;

			SendJsb(writer.write(json));
			std::wstring my_name = nim_ui::UserManager::GetInstance()->GetUserName(nim_ui::LoginManager::GetInstance()->GetAccount(), false);
			AddJsb(jsb, my_name, false);
		}
	}

	void ChatroomForm::AddJsb(const int value, const std::wstring &sender_name, bool is_history)
	{
		std::wstring file_name;
		if (value == 1)
			file_name = L"jsb_s.png";
		else if (value == 2)
			file_name = L"jsb_j.png";
		else if (value == 3)
			file_name = L"jsb_b.png";

		if (file_name.empty()) return;
		// 
		if (msg_list_->GetTextLength() != 0)
		{
			if (is_history)
			{
				msg_list_->SetSel(0, 0);
				msg_list_->ReplaceSel(ROOMMSG_R_N, false);
			}
			else
			{
				msg_list_->SetSel(-1, -1);
				msg_list_->ReplaceSel(ROOMMSG_R_N, false);
			}
		}

		//
		long lSelBegin = 0, lSelEnd = 0;
		CHARFORMAT2 cf;
		ZeroMemory(&cf, sizeof(CHARFORMAT2));
		cf.cbSize = sizeof(cf);
		cf.dwMask = CFM_LINK | CFM_FACE | CFM_COLOR;
		cf.dwEffects = CFE_LINK;

		// 添加发言人，并设置他的颜色
		lSelEnd = lSelBegin = is_history ? 0 : msg_list_->GetTextLength();
		msg_list_->SetSel(lSelEnd, lSelEnd);
		msg_list_->ReplaceSel(sender_name, false);

		lSelEnd = is_history ? sender_name.length() : msg_list_->GetTextLength();
		msg_list_->SetSel(lSelBegin, lSelEnd);
		msg_list_->SetSelectionCharFormat(cf);

		// 设置文本的缩进
		PARAFORMAT2 pf;
		ZeroMemory(&pf, sizeof(PARAFORMAT2));
		pf.cbSize = sizeof(pf);
		pf.dwMask = PFM_STARTINDENT | PFM_LINESPACING;
		LONG lineSpace = (LONG)(1.2 * 20);//设置1.2行间距
		pf.dxStartIndent = 0;
		pf.bLineSpacingRule = 5;
		pf.dyLineSpacing = lineSpace;
		msg_list_->SetParaFormat(pf);
		msg_list_->SetSel(lSelEnd, lSelEnd);
		msg_list_->ReplaceSel(ROOMMSG_R_N, false);
		lSelEnd++;
		// 添加正文，并设置他的颜色	
		msg_list_->SetSel(lSelEnd, lSelEnd);
		std::wstring file = QPath::GetAppPath();
		file.append(L"themes\\default\\session\\" + file_name);
		if (nbase::FilePathIsExist(file, false))
		{
			ITextServices* service = msg_list_->GetTextServices();
			if (!nim_comp::Re_InsertJsb(service, file, file_name))
			{
				QLOG_ERR(L"insert jsb {0} fail") << file;
			}
			service->Release();
		}
		else
		{
			QLOG_ERR(L"jsb {0} miss") << file;
		}

		// 设置正文文本的缩进
		msg_list_->SetSel(lSelEnd, lSelEnd);
		pf.dxStartIndent = 150;
		msg_list_->SetParaFormat(pf);

		if (!is_history)
			msg_list_->EndDown();
		input_edit_->SetFocus();
	}

	void ChatroomForm::SendJsb(const std::string & attach)
	{
		nim_chatroom::ChatRoomMessageSetting setting;
		//{"data":{"value":3},"type":2}
		std::string send_msg = ChatRoom::CreateRoomMessage(kNIMChatRoomMsgTypeCustom, QString::GetGUID(), attach, setting);
		ChatRoom::SendMsg(room_id_, send_msg);

	}

	void ChatroomForm::SendSysNotifyMsg(PushMicNotificationType notifytype, string uid)
	{
		string nick = nbase::UTF16ToUTF8(nim_ui::UserManager::GetInstance()->GetUserName(LoginManager::GetInstance()->GetAccount(), false));
		Json::FastWriter fs;
		Json::Value value;
		Json::Value json;

		if (master_)
		{
			switch (bypass_inact_type_)
			{
			case nim_chatroom::kUnLive:
				break;
			case nim_chatroom::kAudio:
				json["style"] = kAudio;
				break;
			case nim_chatroom::kVedio:
				json["style"] = kVedio;
				break;
			default:
				break;
			}
		}
		else
		{
			if (is_audio_interact_)
				json["style"] = kAudio;
			else
				json["style"] = kVedio;
		}

		//加入连麦队列通知
		json["command"] = notifytype;
		json["info"]["nick"] = nick;
		json["info"]["avatar"] = "avatar_default";
		json["roomid"] = room_id_;
		nim::SysMessage msg;
		msg.receiver_accid_ = uid;
		msg.sender_accid_ = LoginManager::GetInstance()->GetAccount();
		msg.msg_setting_.need_offline_ = nim::BS_FALSE;
		msg.attach_ = fs.write(json);
		msg.type_ = nim::kNIMSysMsgTypeCustomP2PMsg;
		msg.client_msg_id_ = QString::GetGUID();
		msg.timetag_ = 1000 * nbase::Time::Now().ToTimeT();

		nim::SystemMsg::SendCustomNotificationMsg(msg.ToJsonString());
	}

	void ChatroomForm::SendCustomMsg(NTESAnchorOperate opt, std::string uid, InactionType type)
	{
		string nick = nbase::UTF16ToUTF8(nim_ui::UserManager::GetInstance()->GetUserNameW(uid, false));

		Json::FastWriter fw;

		Json::Value value_content;
		value_content["data"]["nick"] = nick;
		value_content["data"]["uid"] = uid;
		value_content["data"]["avatar"] = "avatar_default";
		value_content["data"]["style"] = type;
		value_content["data"]["roomid"] = room_id_;
		value_content["type"] = opt;//2同意连麦，3断开连麦

		string json_msg = fw.write(value_content);
		nim_chatroom::ChatRoomMessageSetting setting;
		std::string send_msg = ChatRoom::CreateRoomMessage(kNIMChatRoomMsgTypeCustom, QString::GetGUID(), json_msg, setting);
		ChatRoom::SendMsg(room_id_, send_msg);
	}

	//主播加入频道成功之后，发一个断开连麦的全局通知
	void ChatroomForm::SendInitMsg()
	{
		Json::FastWriter fw;
		Json::Value value_content;
		value_content["data"]["nick"] = "";
		value_content["data"]["uid"] = NULL;
		value_content["data"]["avatar"] = "avatar_default";
		value_content["data"]["style"] = kAudio;
		value_content["data"]["roomid"] = room_id_;
		value_content["type"] = kNTESAnchorRejectOrDisConnect;//2同意连麦，3断开连麦
		string json_msg = fw.write(value_content);
		nim_chatroom::ChatRoomMessageSetting setting;
		std::string send_msg = ChatRoom::CreateRoomMessage(kNIMChatRoomMsgTypeCustom, QString::GetGUID(), json_msg, setting);
		ChatRoom::SendMsg(room_id_, send_msg);
	}



}