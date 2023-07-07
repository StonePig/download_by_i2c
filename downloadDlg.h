
// downloadDlg.h: 头文件
//

#pragma once

#include "MyProgressCtrl.h"
// CdownloadDlg 对话框
class CdownloadDlg : public CDialogEx
{
// 构造
public:
	CdownloadDlg(CWnd* pParent = nullptr);	// 标准构造函数
	BOOL mClose();
	void enablebtn(BOOL bEnable);

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DOWNLOAD_DIALOG };
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
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnDestroy();
	DECLARE_MESSAGE_MAP()

public:
	BOOL is_downloading;
	CString filePath;
	ULONG mIndex;
	BOOL m_open;
	INT download_time;
	afx_msg void OnBnClickedButtonOpen();
	afx_msg void OnBnClickedButtonDownload();
	CProgressCtrl m_Progress;
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedButtonCheck();
	afx_msg void OnBnClickedButtonReboot();
	CMyProgressCtrl m_myprg;
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};
