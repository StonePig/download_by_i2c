#if !defined(AFX_MYPROGRESSCTRL_H__3069A5BB_5A36_4D4E_B1E3_F703B1F98467__INCLUDED_)
#define AFX_MYPROGRESSCTRL_H__3069A5BB_5A36_4D4E_B1E3_F703B1F98467__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// CMyProgressCtrl window

class CMyProgressCtrl : public CProgressCtrl
{
public:
	CMyProgressCtrl();

	virtual ~CMyProgressCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CMyProgressCtrl)
	afx_msg void OnPaint();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};


#endif // !defined(AFX_MYPROGRESSCTRL_H__3069A5BB_5A36_4D4E_B1E3_F703B1F98467__INCLUDED_)

