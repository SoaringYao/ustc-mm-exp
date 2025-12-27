
// MciTestDlg.h: 头文件
//

#pragma once


// CMciTestDlg 对话框
class CMciTestDlg : public CDialogEx
{
// 构造
public:
	CMciTestDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MCITEST_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonPlaysound();
	afx_msg void OnBnClickedButtonsndplaysound();
	afx_msg void OnBnClickedButtonPlay();
	afx_msg void OnBnClickedButtonStopplay();
	afx_msg void OnBnClickedButtonRecord();
	afx_msg void OnBnClickedButtonStoprecord();
	afx_msg void OnBnClickedOk();
};
