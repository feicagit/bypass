#include "ui_capture_text_edit.h"
#include "capture_def.h"

CCaptureTextEdit::CCaptureTextEdit(const ui::UiRect &rect)
{
	rc_valid_ = rect;
	white_brush_ = CreateSolidBrush(RGB(255, 255, 255));
}

CCaptureTextEdit::~CCaptureTextEdit()
{
	// 释放句柄
	DeleteObject(white_brush_);
}

void CCaptureTextEdit::HandleMessage( ui::EventArgs& event )
{
	// 当编辑框失去焦点的时候
	if(event.Type == ui::kEventKillFocus)
	{
		// 此时鼠标不能在文本编辑框内
		if(!::PtInRect(&rc_valid_, event.ptMouse))
		{
			//TODO::
			return;
		}
	}
	return ui::RichEdit::HandleMessage(event); 
}

void CCaptureTextEdit::Paint( HDC hDC, const ui::UiRect& rcPaint )
{
	DrawRect(hDC); // 绘制矩形虚线
	return __super::Paint(hDC, rcPaint);
}

void CCaptureTextEdit::DrawRect( HDC hdc )
{
	ui::UiRect rcEdit = rc_valid_; // 要绘制的区域 
	InflateRect(&rcEdit, knEditOffset, knEditOffset); // 向外扩展3px
	HPEN dot_pen = CreatePen(PS_DOT, 1, color_pen_);
	HPEN old_pen = (HPEN)SelectObject(hdc, dot_pen);
	HBRUSH old_brush = (HBRUSH)SelectObject(hdc, white_brush_);
	// 绘制白底的虚线框
	Rectangle(hdc, rcEdit.left, rcEdit.top, rcEdit.right, rcEdit.bottom);
	// 还原
	SelectObject(hdc, old_pen);
	SelectObject(hdc, old_brush);
	// 释放
	DeleteObject(dot_pen);
}

