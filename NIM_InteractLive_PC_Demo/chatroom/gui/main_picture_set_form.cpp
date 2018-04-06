#include "main_picture_set_form.h"

const LPTSTR MainPictureSetForm::kClassName = L"MainPictureSetForm";


MainPictureSetForm::MainPictureSetForm()
{
}

MainPictureSetForm::~MainPictureSetForm()
{

}

std::wstring MainPictureSetForm::GetSkinFolder()
{
	return L"chatroom";
}

std::wstring MainPictureSetForm::GetSkinFile()
{
	return L"main_picture_set_form .xml";
}

ui::UILIB_RESOURCETYPE MainPictureSetForm::GetResourceType() const
{
	return ui::UILIB_RESOURCETYPE::UILIB_FILE;
}

std::wstring MainPictureSetForm::GetZIPFileName() const
{
	return L"";
}

std::wstring MainPictureSetForm::GetWindowClassName() const
{
	return kClassName;
}

std::wstring MainPictureSetForm::GetWindowId() const
{
	return kClassName;
}

UINT MainPictureSetForm::GetClassStyle() const
{
	return (UI_CLASSSTYLE_FRAME | CS_DBLCLKS);
}

void MainPictureSetForm::InitWindow()
{
	m_pRoot->AttachBubbledEvent(ui::kEventAll, nbase::Bind(&MainPictureSetForm::Notify, this, std::placeholders::_1));
	m_pRoot->AttachBubbledEvent(ui::kEventClick, nbase::Bind(&MainPictureSetForm::OnClicked, this, std::placeholders::_1));
	m_pRoot->AttachBubbledEvent(ui::kEventSelect, nbase::Bind(&MainPictureSetForm::OnClicked, this, std::placeholders::_1));
	m_pRoot->AttachBubbledEvent(ui::kEventUnSelect, nbase::Bind(&MainPictureSetForm::OnClicked, this, std::placeholders::_1));
	button_ok_ = (ui::Button*)FindControl(L"button_ok");
	button_ok_->SetEnabled(true);
	uid_ = (ui::RichEdit*)FindControl(L"uid");
	uid_->SetEnabled(true);
	uid_->SetText(L"");
}

LRESULT MainPictureSetForm::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return __super::HandleMessage(uMsg, wParam, lParam);
}

bool MainPictureSetForm::Notify(ui::EventArgs * msg)
{
	return true;
}

bool MainPictureSetForm::OnClicked(ui::EventArgs * msg)
{
	std::wstring name = msg->pSender->GetName();
	if (name == L"button_ok")
	{
		OnSetMainPicture();
	}
	return true;
}
void MainPictureSetForm::OnSetMainPicture()
{
	std::wstring uid = uid_->GetText();
	std::string uid_temp = nbase::UTF16ToUTF8(uid);
	auto cb = [](bool ret, int code, const std::string& json_extension)
	{

		StdClosure closure = [=]
		{
			std::wstring prompt_info= L"设置成功";
			if (!ret&&code!=200)
				prompt_info = L"设置失败";

			ShowMsgBox(nullptr, nbase::StringPrintf(L"%s code:%d!", prompt_info.c_str(),code), nullptr, L"提示", L"确定", L"");
		};
		nbase::ThreadManager::PostTask(kThreadUI, closure);


	};
	nim::VChat::SetUidAsMainPicture(uid_temp, "", cb);
}

