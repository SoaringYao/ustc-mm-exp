
// MciTest.cpp: 定义应用程序的类行为。
//

#include "pch.h"
#include "framework.h"
#include "MciTest.h"
#include "MciTestDlg.h"

#include "mmsystem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMciTestApp

BEGIN_MESSAGE_MAP(CMciTestApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CMciTestApp 构造

CMciTestApp::CMciTestApp()
{
	// 支持重新启动管理器
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
	// 在构造函数中暂不做复杂处理，所有重要初始化在 InitInstance 中完成。
	// 如果后续需要增加全局的成员变量初始化，可在此处进行设置。
}


// 唯一的 CMciTestApp 对象

CMciTestApp theApp;


// CMciTestApp 初始化

BOOL CMciTestApp::InitInstance()
{
	// 如果一个运行在 Windows XP 上的应用程序清单指定要
	// 使用 ComCtl32.dll 版本 6 或更高版本来启用可视化方式，
	//则需要 InitCommonControlsEx()。  否则，将无法创建窗口。
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// 将它设置为包括所有要在应用程序中使用的
	// 公共控件类。
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	AfxEnableControlContainer();

	// 创建 shell 管理器，以防对话框包含
	// 任何 shell 树视图控件或 shell 列表视图控件。
	CShellManager* pShellManager = new CShellManager;

	// 激活“Windows Native”视觉管理器，以便在 MFC 控件中启用主题
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// 标准初始化
	// 如果未使用这些功能并希望减小
	// 最终可执行文件的大小，则应移除下列
	// 不需要的特定初始化例程
	// 更改用于存储设置的注册表项，修改为具有实际意义的公司或组织名，避免使用默认字符串
	SetRegistryKey(_T("USTC Multimedia Lab"));

	CMciTestDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// 用户点击“确定”关闭对话框时可在此处执行资源清理
		// 为确保录音和播放设备正确关闭，发送 MCI_CLOSE 指令
		mciSendCommand(MCI_DEVTYPE_WAVEFORM_AUDIO, MCI_CLOSE, 0, NULL);
		mciSendCommand(MCI_DEVTYPE_SEQUENCER, MCI_CLOSE, 0, NULL);
	}
	else if (nResponse == IDCANCEL)
	{
		// 用户点击“取消”关闭对话框时同样需要释放设备
		mciSendCommand(MCI_DEVTYPE_WAVEFORM_AUDIO, MCI_CLOSE, 0, NULL);
		mciSendCommand(MCI_DEVTYPE_SEQUENCER, MCI_CLOSE, 0, NULL);
	}
	else if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "警告: 对话框创建失败，应用程序将意外终止。\n");
		TRACE(traceAppMsg, 0, "警告: 如果您在对话框上使用 MFC 控件，则无法 #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS。\n");
	}

	// 删除上面创建的 shell 管理器。
	if (pShellManager != nullptr)
	{
		delete pShellManager;
	}

#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
	ControlBarCleanUp();
#endif

	// 由于对话框已关闭，所以将返回 FALSE 以便退出应用程序，
	//  而不是启动应用程序的消息泵。
	return FALSE;
}

