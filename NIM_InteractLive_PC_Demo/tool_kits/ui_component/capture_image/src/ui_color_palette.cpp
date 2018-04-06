#include "ui_color_palette.h"
#include "base/util/string_util.h"

/////////////////////////////////////////////////////////////////////////////
//////////////////////////////// 颜色块 /////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

CColorBlock::CColorBlock()
{
	fill_color_brush_ = NULL;
	LOGBRUSH lb = { BS_NULL };
	select_rect_brush_ = CreateBrushIndirect(&lb);
	select_rect_pen_ = CreatePen(PS_SOLID, 2, 0xffffff);
	color_ = 0;
}

CColorBlock::~CColorBlock()
{
	::DeleteObject(fill_color_brush_);
	fill_color_brush_ = NULL;
	::DeleteObject(select_rect_brush_);
	select_rect_brush_ = NULL;
	::DeleteObject(select_rect_pen_);
	select_rect_pen_ = NULL;
}

void CColorBlock::Paint( HDC hDC, const ui::UiRect& rcPaint )
{
	__super::Paint(hDC, rcPaint);
	// 绘制外框
	DrawColorBlock(hDC, m_rcItem);
}

void CColorBlock::DrawColorBlock( HDC hdc, const ui::UiRect &rect )
{
	if(fill_color_brush_)
	{
		ui::UiRect block_rect = rect;
		block_rect.Deflate(1, 1);
		::FillRect(hdc, &block_rect, fill_color_brush_);
		if (select_rect_pen_ && m_bSelected)
		{
			HPEN old_pen = (HPEN)SelectObject(hdc, select_rect_pen_);
			HBRUSH old_brush = (HBRUSH)SelectObject(hdc, select_rect_brush_);
			::Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
			SelectObject(hdc, old_brush);
			SelectObject(hdc, old_pen);
		}
	}	
}

void CColorBlock::SetAttribute( const std::wstring& pstrName, const std::wstring& pstrValue )
{
	if(pstrName == _T("color"))
	{
		LPCTSTR pValue = pstrValue.c_str();
		while( *pValue > _T('\0') && *pValue <= _T(' ') ) pValue = ::CharNext(pValue);
		std::wstring strValue = ui::GlobalManager::GetTextColor(pValue);
		strValue = strValue.substr(1);
		LPTSTR pstr = NULL;
		color_ = _tcstoul(strValue.c_str(), &pstr, 16);
		color_ = (0xff & color_) << 16 | (0xff00 & color_) | (0xff0000 & color_) >> 16;
		fill_color_brush_ = CreateSolidBrush(color_);
	}
	else __super::SetAttribute(pstrName, pstrValue);
}

/////////////////////////////////////////////////////////////////////////////
//////////////////////////////// 画刷板 /////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

CBrushBlock::CBrushBlock()
{
	brush_width_ = 0;
	brush_width2_ = 0;
	brush_paint_width_ = 0;
	brush_.reset(NULL);
}

CBrushBlock::~CBrushBlock()
{

}

void CBrushBlock::SetAttribute( const std::wstring& pstrName, const std::wstring& pstrValue )
{
	if(pstrName == _T("brush_width"))
	{
		brush_width_ = _ttoi(pstrValue.c_str());
	}
	else if (pstrName == _T("brush_width2"))
	{
		brush_width2_ = _ttoi(pstrValue.c_str());
	}
	else if (pstrName == _T("brush_paint_width"))
	{
		brush_paint_width_ = _ttoi(pstrValue.c_str());
	}
	else __super::SetAttribute(pstrName, pstrValue);
}

void CBrushBlock::Paint( HDC hDC, const ui::UiRect& rcPaint )
{
	__super::Paint(hDC, rcPaint);
	int width = m_rcItem.right - m_rcItem.left;
	int offset = (width - brush_paint_width_) / 2;
	RECT rect = { m_rcItem.left + offset, m_rcItem.top + offset, rect.left + brush_paint_width_, rect.top + brush_paint_width_ };
	DrawCircleBlock(hDC, rect);
}

void CBrushBlock::HandleMessage( ui::EventArgs& event )
{
	ui::Option::HandleMessage(event);
}

void CBrushBlock::DrawCircleBlock(HDC hdc, const ui::UiRect &rect)
{
	if(brush_.get() == NULL)
		brush_.reset(new Gdiplus::SolidBrush(Gdiplus::Color::MakeARGB(0xff, 0xd8, 0xe1, 0xeb)));

	if(brush_.get())
	{
		using namespace Gdiplus;
		Graphics graphic(hdc);
		graphic.SetSmoothingMode(SmoothingModeAntiAlias);
#if (GDIPVER >= 0x0110)
		graphic.SetSmoothingMode(SmoothingModeAntiAlias8x8);
#endif
		graphic.SetCompositingMode(CompositingModeSourceOver);
		graphic.SetInterpolationMode(InterpolationModeHighQuality);
		graphic.SetPixelOffsetMode(PixelOffsetModeHighQuality);

		Rect fill_rect(rect.left, rect.top, rect.GetWidth(), rect.GetHeight());
		graphic.FillEllipse(brush_.get(), fill_rect);
	}
}

/////////////////////////////////////////////////////////////////////////////
//////////////////////////////// 调色板 /////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#define MIN_BRUSH_WIDTH 1
#define MIN_BRUSH_WIDTH2 10

CColorPalette::CColorPalette()
{
	color_palette_ = NULL;
	selected_color_ = NULL;
	selected_brush_width_ = MIN_BRUSH_WIDTH; // 默认2px
	selected_brush_width2_ = MIN_BRUSH_WIDTH2;
	SetFixedWidth(DUI_LENGTH_AUTO);
	SetFixedHeight(DUI_LENGTH_AUTO);
	color_ = 0;
}

CColorPalette::~CColorPalette()
{

}

ui::Box* CColorPalette::CreateColorPaletteUI(ui::CreateControlCallback callback, bool visible /*=true*/)
{
	color_palette_ = ui::GlobalManager::CreateBox(L"capture_image/color_palette.xml", callback);
	color_palette_->SetVisible(visible);
	this->Add(color_palette_);
	return this;
}
void CColorPalette::ShowRange(bool show)
{
	ui::Box* range_box = (ui::Box*)m_pWindow->FindControl(L"range_box");
	if (range_box)
	{
		range_box->SetVisible(show);
	}
	ui::Box* color_block = (ui::Box*)m_pWindow->FindControl(L"color_block");
	if (color_block)
	{
		color_block->SetVisible(!show);
	}
}

void CColorPalette::InitChildControls()
{
	if(m_pWindow)
	{
		selected_color_ = static_cast<CColorBlock*>(m_pWindow->FindControl(L"select_color"));
		std::wstring ctrl_name;
		CColorBlock* color_block_ctrl = NULL;
		CBrushBlock* brush_ctrl = NULL;
		for(int i =1; i <= 9; i++)
		{
			nbase::StringPrintf(ctrl_name, L"color%d", i);
			color_block_ctrl = static_cast<CColorBlock*>(m_pWindow->FindControl(ctrl_name.c_str()));
			if(color_block_ctrl)
			{
				color_block_ctrl->AttachSelect(nbase::Bind(&CColorPalette::OnClick, this, std::placeholders::_1)); // 添加点击响应
				if (i==1)
				{
					color_ = color_block_ctrl->GetColor();
					color_block_ctrl->Selected(true);
				}
			}
		}
		for(int i = 1; i <=3; i++)
		{
			nbase::StringPrintf(ctrl_name, L"brush%d", i);
			brush_ctrl = static_cast<CBrushBlock*>(m_pWindow->FindControl(ctrl_name.c_str()));
			if(brush_ctrl)
			{
				brush_ctrl->AttachSelect(nbase::Bind(&CColorPalette::OnSelect, this, std::placeholders::_1)); // 单选响应
			}
		}
		ui::Slider* range = static_cast<ui::Slider*>(m_pWindow->FindControl(L"range"));
		if (range)
		{
			range->SetValue(5);
		}
	}
}

bool CColorPalette::OnSelect( ui::EventArgs *msg )
{
	std::wstring sender_name = msg->pSender->GetName();

	if(sender_name.find(L"brush") == 0)
	{
		// 获取画刷的宽度
		selected_brush_width_ = ((CBrushBlock*)msg->pSender)->GetBrushWidth();
		selected_brush_width2_ = ((CBrushBlock*)msg->pSender)->GetBrushWidth2();
	}

	return true;
}

bool CColorPalette::OnClick(ui::EventArgs* param)
{
	std::wstring sender_name = param->pSender->GetName();
	if(sender_name.find(L"color") == 0)
	{
		// 获取颜色名称
		color_ = ((CColorBlock*)param->pSender)->GetColor();
	}

	return true;
}
DWORD CColorPalette::GetSelectedColorRGB()
{
	return color_;
}

int CColorPalette::GetSelectedBrushWidth()
{
	return (selected_brush_width_ >= MIN_BRUSH_WIDTH)? selected_brush_width_ : MIN_BRUSH_WIDTH;
}
int CColorPalette::GetSelectedBrushWidth2()
{
	return (selected_brush_width2_ >= MIN_BRUSH_WIDTH2) ? selected_brush_width2_ : MIN_BRUSH_WIDTH2;
}
int CColorPalette::GetRangePos()
{
	ui::Slider* range = static_cast<ui::Slider*>(m_pWindow->FindControl(L"range"));
	if (range)
	{
		return range->GetValue();
	}
	return 1;
}
int CColorPalette::GetToolbarHeight()
{
	if (color_palette_)
	{
		return color_palette_->GetFixedHeight();
	}
	return 0;
}

