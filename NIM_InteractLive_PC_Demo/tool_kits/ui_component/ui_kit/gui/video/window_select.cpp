#include "window_select.h"
#include "helper/screen_capture.h"
#include "base/util/string_number_conversions.h"
#include "shared\ui\msgbox.h"

using namespace nim_comp;
const LPCTSTR WindowSelectForm::kClassName = L"WindowSelectForm";

WindowSelectForm::WindowSelectForm()
{
	select_wnd_cb_ = nullptr;
}

WindowSelectForm::~WindowSelectForm()
{
}

std::wstring WindowSelectForm::GetSkinFolder()
{
	return L"video";
}

std::wstring WindowSelectForm::GetSkinFile()
{
	return L"wnd_select.xml";
}

ui::UILIB_RESOURCETYPE WindowSelectForm::GetResourceType() const
{
	return ui::UILIB_RESOURCETYPE::UILIB_FILE;
}

std::wstring WindowSelectForm::GetZIPFileName() const
{
	return L"";
}

std::wstring WindowSelectForm::GetWindowClassName() const
{
	return kClassName;
}

std::wstring WindowSelectForm::GetWindowId() const
{
	return kClassName;
}

UINT WindowSelectForm::GetClassStyle() const
{
	return (UI_CLASSSTYLE_FRAME | CS_DBLCLKS);
}

void WindowSelectForm::InitWindow()
{
	m_pRoot->AttachBubbledEvent(ui::kEventAll, nbase::Bind(&WindowSelectForm::Notify, this, std::placeholders::_1));
	m_pRoot->AttachBubbledEvent(ui::kEventClick, nbase::Bind(&WindowSelectForm::OnClicked, this, std::placeholders::_1));
	m_pRoot->AttachBubbledEvent(ui::kEventSelect, nbase::Bind(&WindowSelectForm::OnSelected, this, std::placeholders::_1));
	m_pRoot->AttachBubbledEvent(ui::kEventUnSelect, nbase::Bind(&WindowSelectForm::OnSelected, this, std::placeholders::_1));

	wnd_list_ = (ui::ListBox*)FindControl(L"wnd_list");
	RefreshWndlist();
}

LRESULT WindowSelectForm::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return __super::HandleMessage(uMsg, wParam, lParam);
}

bool WindowSelectForm::Notify(ui::EventArgs * msg)
{
	return false;
}

bool WindowSelectForm::OnClicked(ui::EventArgs * msg)
{
	std::wstring name = msg->pSender->GetName();
	uint32_t id = 0;
	if (name == L"select")
	{
		int i = 0;
		if (select_wnd_cb_)
		{

			for (i = 0; i < wnd_list_->GetCount(); ++i)
			{
				ui::ListContainerElement* item = (ui::ListContainerElement*)wnd_list_->GetItemAt(i);
				if (item->IsSelected())
				{
					nbase::StringToUint(item->GetDataID(), &id);
					if (id == 0)
					{
						select_wnd_cb_(GetDesktopWindow(), true);
					}
					else
					{
						select_wnd_cb_((HWND)id, false);
					}
					break;
				}
			}
			if (i >= wnd_list_->GetCount())
			{
				ShowMsgBox(m_hWnd, L"请选择一项！", MsgboxCallback(), L"提示", L"确定", L"");
			}
			else
			{
				Close();
			}
		}
	}
	else if (name == L"close")
	{
		Close();
	}
	return true;
}

bool WindowSelectForm::OnSelected(ui::EventArgs * msg)
{
	std::wstring name = msg->pSender->GetName();
	if (name == L"manage_room")
	{

	}
	else if (name == L"apply_room")
	{

	}

	return false;
}

void WindowSelectForm::RefreshWndlist()
{
	wnd_list_->RemoveAll();
	nim_comp::CaptureWindowInfoList wnd_list;
	nim_comp::ScreenCapture::GetCaptureWindowList(&wnd_list);
	for (auto& it : wnd_list)
	{
		ui::ListContainerElement* item = (ui::ListContainerElement*)ui::GlobalManager::CreateBoxWithCache(L"video/wnd_item.xml");
		if (item)
		{
			ui::Label* wnd_name = (ui::Label*)item->FindSubControl(L"wnd_name");
			if (wnd_name)
			{
				wnd_name->SetText(it.title);
			}
			item->SetDataID(nbase::UintToString16((uint32_t)it.id));
			wnd_list_->Add(item);
		}
	}
}


