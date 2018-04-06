#include "audio_hook_form.h"

const LPTSTR AudioHookForm::kClassName = L"AudioHookForm";

bool open_audio_hook_ = false;
std::wstring audio_path_;

AudioHookForm::AudioHookForm()
{
}

AudioHookForm::~AudioHookForm()
{

}

std::wstring AudioHookForm::GetSkinFolder()
{
	return L"chatroom";
}

std::wstring AudioHookForm::GetSkinFile()
{
	return L"audio_hook_form.xml";
}

ui::UILIB_RESOURCETYPE AudioHookForm::GetResourceType() const
{
	return ui::UILIB_RESOURCETYPE::UILIB_FILE;
}

std::wstring AudioHookForm::GetZIPFileName() const
{
	return L"";
}

std::wstring AudioHookForm::GetWindowClassName() const
{
	return kClassName;
}

std::wstring AudioHookForm::GetWindowId() const
{
	return kClassName;
}

UINT AudioHookForm::GetClassStyle() const
{
	return (UI_CLASSSTYLE_FRAME | CS_DBLCLKS);
}

void AudioHookForm::InitWindow()
{
	m_pRoot->AttachBubbledEvent(ui::kEventAll, nbase::Bind(&AudioHookForm::Notify, this, std::placeholders::_1));
	m_pRoot->AttachBubbledEvent(ui::kEventClick, nbase::Bind(&AudioHookForm::OnClicked, this, std::placeholders::_1));
	m_pRoot->AttachBubbledEvent(ui::kEventSelect, nbase::Bind(&AudioHookForm::OnClicked, this, std::placeholders::_1));
	m_pRoot->AttachBubbledEvent(ui::kEventUnSelect, nbase::Bind(&AudioHookForm::OnClicked, this, std::placeholders::_1));
	open_btn_ = (ui::CheckBox*)FindControl(L"open");
	open_btn_->Selected(open_audio_hook_);
	button_ok_ = (ui::Button*)FindControl(L"button_ok");
	button_ok_->SetEnabled(open_audio_hook_);
	edit_ = (ui::RichEdit*)FindControl(L"edit");
	edit_->SetEnabled(open_audio_hook_);
	edit_->SetText(audio_path_);
}

LRESULT AudioHookForm::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return __super::HandleMessage(uMsg, wParam, lParam);
}

bool AudioHookForm::Notify(ui::EventArgs * msg)
{
	return true;
}

bool AudioHookForm::OnClicked(ui::EventArgs * msg)
{
	std::wstring name = msg->pSender->GetName();
	if (name == L"button_ok")
	{
		StartAudio();
	}
	else if (name == L"open")
	{
		open_audio_hook_ = false;
		if (open_btn_->IsSelected())
		{
			open_audio_hook_ = true;
		}
		else
		{
			nim::VChat::EndDevice(nim::kNIMDeviceTypeAudioHook);
		}
		button_ok_->SetEnabled(open_audio_hook_);
		edit_->SetEnabled(open_audio_hook_);
	}
	return true;
}
void AudioHookForm::StartAudio()
{
	std::wstring path = edit_->GetText();
	std::string path_temp = nbase::UTF16ToUTF8(path);
	bool error = false;
	if (nbase::FilePathIsExist(path, false) && !nbase::FilePathIsExist(path, true))
	{
		audio_path_ = path;
		nim::VChat::EndDevice(nim::kNIMDeviceTypeAudioHook);
		auto cb = [](nim::NIMDeviceType type, bool ret, const char *json_extension, const void *user_data)
		{
			if (!ret)
			{
				StdClosure closure = []{
					ShowMsgBox(nullptr, L"打开背景乐失败！", nullptr, L"提示", L"确定", L"");
				};
				nbase::ThreadManager::PostTask(kThreadUI, closure);
			}
		};
		nim::VChat::StartDevice(nim::kNIMDeviceTypeAudioHook, path_temp, 0, 0, 0, cb);
	}
	else
	{
		error = true;
	}
	ui::Control *tip_err = FindControl(L"tip_err");
	if (tip_err)
	{
		tip_err->SetVisible(error);
	}
	if (!error)
	{
		Close();
	}
}
void AudioHookForm::ResetAudioHookType()
{
	open_audio_hook_ = false;
}

bool AudioHookForm::GetAudioHookType()
{
	return open_audio_hook_;
}

std::wstring AudioHookForm::GetAudioHookPath()
{
	return  audio_path_;
}

