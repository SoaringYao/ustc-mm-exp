
// MciTestDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "MciTest.h"
#include "MciTestDlg.h"
#include "afxdialogex.h"

#include "mmsystem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

WORD wDeviceID;


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMciTestDlg 对话框



CMciTestDlg::CMciTestDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MCITEST_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMciTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMciTestDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_PlaySound, &CMciTestDlg::OnBnClickedButtonPlaysound)
	ON_BN_CLICKED(IDC_BUTTON_sndPlaySound, &CMciTestDlg::OnBnClickedButtonsndplaysound)
	ON_BN_CLICKED(IDC_BUTTON_Play, &CMciTestDlg::OnBnClickedButtonPlay)
	ON_BN_CLICKED(IDC_BUTTON_StopPlay, &CMciTestDlg::OnBnClickedButtonStopplay)
	ON_BN_CLICKED(IDC_BUTTON_Record, &CMciTestDlg::OnBnClickedButtonRecord)
	ON_BN_CLICKED(IDC_BUTTON_StopRecord, &CMciTestDlg::OnBnClickedButtonStoprecord)
	ON_BN_CLICKED(IDOK, &CMciTestDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CMciTestDlg 消息处理程序

BOOL CMciTestDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CMciTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CMciTestDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CMciTestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CMciTestDlg::OnBnClickedButtonPlaysound()
{
	PlaySound(_T("C:\\Windows\\Media\\Ring01.wav"), NULL, SND_SYNC);
}


void CMciTestDlg::OnBnClickedButtonsndplaysound()
{
	sndPlaySound(_T("C:\\Windows\\Media\\Ring01.wav"), SND_SYNC);
}

//procedure for play a midi
void CMciTestDlg::OnBnClickedButtonPlay()
{
	HWND hwnd;
	MCI_OPEN_PARMS mciopen;
	MCI_OPEN_PARMS mciplay;
	DWORD rtrn;
	char b[80];
	hwnd = GetActiveWindow()->m_hWnd;
//	mciopen.lpstrElementName = _T("E:\\tmp\\onestop.mid");
	mciopen.lpstrElementName = _T("C:\\Windows\\Media\\onestop.mid");
	mciopen.lpstrDeviceType = _T("sequencer");

	// CXH20240726，WIN11下测试，需要使用 DWORD_PTR 类型指针
	// 否则会发生异常，类似：0x00007FFCD1B62687 (winmm.dll)处(位于 MciTest.exe 中)引发的异常: 0xC0000005: 读取位置 0x000000003D8FD904 时发生访问冲突。
	/*  https://learn.microsoft.com/en-us/windows/win32/winprog/windows-data-types
		DWORD	A 32-bit unsigned integer. 
		DWORD_PTR	An unsigned long type for pointer precision. Use when casting a pointer to a long type to perform pointer arithmetic. (Also commonly used for general 32-bit parameters that have been extended to 64 bits in 64-bit Windows.)
	*/
	rtrn = mciSendCommand(NULL, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_ELEMENT, (DWORD_PTR)&mciopen);
	//rtrn = mciSendCommand(NULL, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_ELEMENT, (DWORD)(LPVOID)&mciopen);
	if (rtrn != 0) {
		mciGetErrorString(rtrn, (LPSTR)b, 80);
		::MessageBox(hwnd, (LPSTR)b, _T("MCI ERROR!"), MB_OK);
	}
	wDeviceID = mciopen.wDeviceID;
	mciplay.dwCallback = (DWORD)hwnd;
	rtrn = mciSendCommand(wDeviceID, MCI_PLAY, MCI_NOTIFY, (DWORD_PTR)&mciplay);
	//rtrn = mciSendCommand(wDeviceID, MCI_PLAY, MCI_NOTIFY, (DWORD)(LPVOID)&mciplay);

	if (rtrn != 0L) {
		mciGetErrorString(rtrn, (LPSTR)b, 80);
		::MessageBox(hwnd, (LPSTR)b, _T("MCI Error"), MB_OK);
		mciSendCommand(wDeviceID, MCI_CLOSE, 0, NULL);
	}
}


void CMciTestDlg::OnBnClickedButtonStopplay()
{
	HWND hwnd;
	MCI_GENERIC_PARMS mcistop;

	hwnd = GetActiveWindow()->m_hWnd;
	mcistop.dwCallback = (DWORD)hwnd;
	mciSendCommand(MCI_DEVTYPE_SEQUENCER, MCI_STOP, 0, (DWORD_PTR)&mcistop);
	//mciSendCommand(MCI_DEVTYPE_SEQUENCER, MCI_STOP, 0, (DWORD)(LPVOID)&mcistop);
	mciSendCommand(wDeviceID, MCI_STOP, 0, (DWORD_PTR)&mcistop);
	//mciSendCommand(wDeviceID, MCI_STOP, 0, (DWORD)(LPVOID)&mcistop);
}

//record
void CMciTestDlg::OnBnClickedButtonRecord()
{
	HWND hwnd;
	MCI_OPEN_PARMS mciopen;
	MCI_RECORD_PARMS mci1;
	MCI_SAVE_PARMS mcisave;
	DWORD rtrn;
	char b[80];

	hwnd = GetActiveWindow()->m_hWnd;
//	TCHAR szPath[MAX_PATH];
//	GetCurrentDirectory(MAX_PATH, szPath);
//	AfxMessageBox(szPath);
	mciopen.lpstrElementName = "E:\\_project\\VisualStudio\\tmp\\Classic.wav";
	//先拷贝一个已有的wav文件，更名为 eeis.wav。录音时会覆盖eeis.wav
	mcisave.lpfilename = "E:\\_project\\VisualStudio\\tmp\\Classic.wav";
	mciopen.lpstrDeviceType = "waveaudio";


	rtrn = mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_ELEMENT, (DWORD_PTR)&mciopen);
	//rtrn = mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_ELEMENT, (DWORD)(LPVOID)&mciopen);
	if (rtrn != 0) {
		mciGetErrorString(rtrn, (LPSTR)b, 80);
		::MessageBox(hwnd, b, "MCI ERROR!", MB_OK);
	}
	wDeviceID = mciopen.wDeviceID;
	mci1.dwCallback = (DWORD)hwnd;
	rtrn = mciSendCommand(wDeviceID, MCI_RECORD, MCI_NOTIFY, (DWORD_PTR)&mci1);
	//rtrn = mciSendCommand(wDeviceID, MCI_RECORD, MCI_NOTIFY, (DWORD)(LPVOID)&mci1);

	if (rtrn != 0L) {
		mciGetErrorString(rtrn, (LPSTR)b, 80);
		::MessageBox(hwnd, b, "MCI Error", MB_OK);
		mciSendCommand(wDeviceID, MCI_CLOSE, 0, NULL);
	}
}

//save and stop record
void CMciTestDlg::OnBnClickedButtonStoprecord()
{
	HWND hwnd;
	MCI_SAVE_PARMS mcisave;
	MCI_OPEN_PARMS mciopen;
	DWORD rtrn;
	char b[80];
	hwnd = GetActiveWindow()->m_hWnd;
	mciopen.lpstrElementName = "E:\\_project\\VisualStudio\\tmp\\Classic.wav";
	mcisave.lpfilename = "E:\\_project\\VisualStudio\\tmp\\Classic.wav";
	mciopen.lpstrDeviceType = "waveaudio";

	mcisave.dwCallback = (DWORD)hwnd;
	rtrn = mciSendCommand(wDeviceID, MCI_SAVE, MCI_NOTIFY, (DWORD_PTR)&mcisave);
	//rtrn = mciSendCommand(wDeviceID, MCI_SAVE, MCI_NOTIFY, (DWORD)(LPVOID)&mcisave);

	if (rtrn != 0L) {
		mciGetErrorString(rtrn, (LPSTR)b, 80);
		::MessageBox(hwnd, b, "MCI Error save", MB_OK);
		mciSendCommand(wDeviceID, MCI_CLOSE, 0, NULL);
	}
}


void CMciTestDlg::OnBnClickedOk()
{
	mciSendCommand(MCI_DEVTYPE_WAVEFORM_AUDIO, MCI_CLOSE, 0, NULL);
	mciSendCommand(MCI_DEVTYPE_SEQUENCER, MCI_CLOSE, 0, NULL);

	CDialogEx::OnOK();
}
