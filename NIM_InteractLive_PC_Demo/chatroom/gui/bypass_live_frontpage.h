#pragma once
#include "chatroom_form.h"
#include "nim_cpp_vchat.h"

namespace nim_chatroom
{

class BypassLiveFontPage : public nim_comp::WindowEx
{
public:
	BypassLiveFontPage();
	~BypassLiveFontPage();

	virtual std::wstring GetSkinFolder() override;
	virtual std::wstring GetSkinFile() override;
	virtual ui::UILIB_RESOURCETYPE GetResourceType() const;
	virtual std::wstring GetZIPFileName() const;

	virtual std::wstring GetWindowClassName() const override;
	virtual std::wstring GetWindowId() const override;
	virtual UINT GetClassStyle() const override;
	virtual LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) override;
	virtual void InitWindow() override;

	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	bool Notify(ui::EventArgs* msg);
	bool OnClicked(ui::EventArgs* msg);

	bool OnSelVedioModel(ui::EventArgs* param);
	bool OnSelAudioModel(ui::EventArgs* param);

	static void SetNetDetectInfo(nim::NetDetectCbInfo info);
	void WindowReset() { btn_create_room_->SetEnabled(true);btn_join_room_->SetEnabled(true); }
private:
	void JoinRoom(int64_t id);
	void CreateMyRoomInfo();
	void PopupMainMenu(POINT point);
	void NetDetect();
	void NetDetectCb(int code, nim::NetDetectCbInfo info);


	

public:
	static const LPTSTR kClassName;

private:
	ui::RichEdit* id_edit_;
	ui::Option* video_model_;
	ui::Option* audio_model_;
	//ui::RichEdit* name_edit_;
	bool create_room_ = false;
	InactionType livingmodel_;
	AutoUnregister	unregister_cb;

	//ÍøÂç¼ì²â
	ui::Box* net_detect_type_;
	ui::Control* net_detect_loading_;
	ui::TabBox* net_detect_step_;
	ui::Box* net_detect_tip_;
	ui::ButtonBox* net_detect_info_;
	ui::Label* tip_t_0_;
	ui::Label* tip_t_1_;
	ui::Label* tip_t_2_;
	ui::Label* tip_t_3_;
	ui::Label* tip_t_4_;
	ui::Button* btn_create_room_;
	ui::Button* btn_join_room_;

};
}