#include "main.h"
#include "resource.h"
#include "gui/login/login_form.h"
#include "app_dump.h"
#include "base/util/at_exit.h"
#include "shared/xml_util.h"
#include "base/util/string_number_conversions.h"
#include "callback/chatroom_callback.h"

void MainThread::Init()
{
	nbase::ThreadManager::RegisterThread(kThreadUI);
	PreMessageLoop();

	std::wstring theme_dir = QPath::GetAppPath();
	ui::GlobalManager::Startup(theme_dir + L"themes\\default", ui::CreateControlCallback());

	nim_ui::UserConfig::GetInstance()->SetIcon(IDI_ICON);

	std::wstring app_crash = QCommand::Get(kCmdAppCrash);
	if( app_crash.empty() )
	{	
		nim_ui::WindowsManager::SingletonShow<LoginForm>(LoginForm::kClassName);
	}
	else
	{
		std::wstring content(L"��������ˣ�������־��");
		content.append(app_crash);

		MsgboxCallback cb = nbase::Bind(&MainThread::OnMsgBoxCallback, this, std::placeholders::_1);
		ShowMsgBox(NULL, content, cb, L"��ʾ", L"��", L"ȡ��");
	}
}

void MainThread::Cleanup()
{
	ui::GlobalManager::Shutdown();

	PostMessageLoop();
	SetThreadWasQuitProperly(true);
	nbase::ThreadManager::UnregisterThread();

	nim_chatroom::ChatRoom::Cleanup();
}

void MainThread::PreMessageLoop()
{
	misc_thread_.reset( new MiscThread(kThreadGlobalMisc, "Global Misc Thread") );
	misc_thread_->Start();

	ls_thread_.reset(new MiscThread(kThreadLiveStreaming, "LiveStreaming Thread"));
	ls_thread_->Start();

	player_thread_.reset(new MiscThread(kThreadLivePlaying, "LivePlaying Thread"));
	player_thread_->Start();

	screen_capture_thread_.reset(new MiscThread(kThreadScreenCapture, "ScreenCapture Thread"));
	screen_capture_thread_->Start();

	db_thread_.reset( new DBThread(kThreadDatabase, "Database Thread") );
	db_thread_->Start();
}

void MainThread::PostMessageLoop()
{
	misc_thread_->Stop();
	misc_thread_.reset(NULL);

	ls_thread_->Stop();
	ls_thread_.reset(NULL);

	player_thread_->Stop();
	player_thread_.reset(NULL);

	screen_capture_thread_->Stop();
	screen_capture_thread_.reset(NULL);

	db_thread_->Stop();
	db_thread_.reset(NULL);
}

void MainThread::OnMsgBoxCallback( MsgBoxRet ret )
{
	if(ret == MB_YES)
	{
		std::wstring dir = QPath::GetNimAppDataDir();
		QCommand::AppStartWidthCommand(dir, L"");
	}

	nim_ui::WindowsManager::SingletonShow<LoginForm>(LoginForm::kClassName);
}

/**
* ȫ�ֺ�������ʼ�����š�������ȡӦ�÷�������ַ����������sdk����ʼ����װĿ¼���û�Ŀ¼��ע���յ�����ʱִ�еĻص�������
* @return void �޷���ֵ
*/
static void InitNim()
{
	std::wstring server_conf_path = QPath::GetAppPath();
	server_conf_path.append(L"server_conf.txt");
	nim::SDKConfig config;
	TiXmlDocument document;
	if (shared::LoadXmlFromFile(document, server_conf_path))
	{
		TiXmlElement* root = document.RootElement();
		if (root)
		{
			bool use_private_server = false;
			Json::Value srv_config;
			if (auto pchar = root->Attribute("kNIMLbsAddress")) {
				config.lbs_address_ = pchar;
				use_private_server = true;
			}
			if (auto pchar = root->Attribute("kNIMNosLbsAddress")) {
				config.nos_lbs_address_ = pchar;
				use_private_server = true;
			}
			if (auto pchar = root->Attribute("kNIMDefaultLinkAddress")) {
				config.default_link_address_ = pchar;
				use_private_server = true;
			}
			if (auto pchar = root->Attribute("kNIMDefaultNosUploadAddress")) {
				config.default_nos_upload_address_=pchar;
				use_private_server = true;
			}
			if (auto pchar = root->Attribute("kNIMDefaultNosDownloadAddress")) {
				config.default_nos_download_address_ = pchar;
			}
			if (auto pchar = root->Attribute("kNIMDefaultNosAccessAddress")) {
				config.default_nos_access_address_ = pchar;
				use_private_server = true;
			}
			if (auto pchar = root->Attribute("kNIMRsaPublicKeyModule")) {
				config.rsa_public_key_module_ = pchar;
				use_private_server = true;
			}
			if (auto pchar = root->Attribute("kNIMRsaVersion")) {
				nbase::StringToInt((std::string)pchar, &config.rsa_version_);
				use_private_server = true;
			}
			config.use_private_server_ = use_private_server;
		}
	}

	//sdk�������������
	config.database_encrypt_key_ = "Netease"; //string��db key���Ŀǰֻ֧�����32���ַ��ļ�����Կ������ʹ��32���ַ���

	std::string app_key = GetConfigValueAppKey();
	bool ret = nim::Client::Init(app_key, "NIM_LIVE", "", config); // ��������sdk����ʼ����װĿ¼���û�Ŀ¼
	assert(ret);
	ret = nim_chatroom::ChatRoom::Init("","");
	assert(ret);

	nim_ui::InitManager::GetInstance()->InitUiKit();
	nim_chatroom::ChatroomCallback::InitChatroomCallback();
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPWSTR lpszCmdLine, int nCmdShow)
{
	nbase::AtExitManager at_manager;

	CComModule _Module;
	_Module.Init(NULL, hInst);

	_wsetlocale(LC_ALL, L"chs");

#ifdef _DEBUG
	AllocConsole();
	FILE* fp = NULL;
	freopen_s(&fp, "CONOUT$", "w+t", stdout);
	wprintf_s(L"Command:\n%s\n\n", lpszCmdLine);
#endif

	srand( (unsigned int) time(NULL) );

	::SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);

	QCommand::ParseCommand(lpszCmdLine);

	HRESULT hr = ::OleInitialize(NULL);
	if( FAILED(hr) )
		return 0;

	InitNim(); // ��ʼ�����ź�UI���
	{
		MainThread thread; // �������߳�
		thread.RunOnCurrentThreadWithLoop(nbase::MessageLoop::kUIMessageLoop); // ִ�����߳�ѭ��
	}
	QLOG_APP(L"exit ui loop");

	// �������֮ǰ����������sdk��UI���
	nim_ui::InitManager::GetInstance()->CleanupUiKit();

	QLOG_APP(L"app exit");

	// �Ƿ��������г���
	std::wstring restart = QCommand::Get(kCmdRestart);
	if( !restart.empty() )
	{
		std::wstring cmd;
		std::wstring acc = QCommand::Get(kCmdAccount);
		if( !acc.empty() )
			cmd.append( nbase::StringPrintf(L" /%s %s ", kCmdAccount.c_str(), acc.c_str()) );
		std::wstring exit_why = QCommand::Get(kCmdExitWhy);
		if( !exit_why.empty() )
			cmd.append( nbase::StringPrintf(L" /%s %s ", kCmdExitWhy.c_str(), exit_why.c_str()) );
		QCommand::RestartApp(cmd);
	}

	_Module.Term();
	::OleUninitialize();

	return 0;
}
