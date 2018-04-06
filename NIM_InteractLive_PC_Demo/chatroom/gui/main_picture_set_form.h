#pragma once

class MainPictureSetForm : public nim_comp::WindowEx
{
public:
	MainPictureSetForm();
	~MainPictureSetForm();

	virtual std::wstring GetSkinFolder() override;
	virtual std::wstring GetSkinFile() override;
	virtual ui::UILIB_RESOURCETYPE GetResourceType() const;
	virtual std::wstring GetZIPFileName() const;

	virtual std::wstring GetWindowClassName() const override;
	virtual std::wstring GetWindowId() const override;
	virtual UINT GetClassStyle() const override;

	virtual void InitWindow() override;
	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	virtual bool Notify(ui::EventArgs* msg);
	virtual bool OnClicked(ui::EventArgs* msg);

public:
	void OnSetMainPicture();

private:
	ui::Button *button_ok_;
	ui::RichEdit *uid_;

public:
	static const LPTSTR kClassName;

};
