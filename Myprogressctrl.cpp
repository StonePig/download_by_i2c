// MyProgressCtrl.cpp : implementation file
//
#include "pch.h"
//#include "stdafx.h"
#include "Myprogressctrl.h"

// CMyProgressCtrl

CMyProgressCtrl::CMyProgressCtrl()
{
}

CMyProgressCtrl::~CMyProgressCtrl()
{
}


BEGIN_MESSAGE_MAP(CMyProgressCtrl, CProgressCtrl)
	//{{AFX_MSG_MAP(CMyProgressCtrl)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

COLORREF GetColor(int nPos, int nMax)
{
	BYTE r1 = 101, g1 = 162, b1 = 225;
	BYTE r2 = 82, g2 = 134, b2 = 185;

	if (nPos < nMax / 2)
	{
		nMax = nMax / 2;
		BYTE r = r1 + (r2 - r1) * nPos / nMax;
		BYTE g = g1 + (g2 - g1) * nPos / nMax;
		BYTE b = b1 + (b2 - b1) * nPos / nMax;

		return RGB(r, g, b);
	}

	nMax = nMax / 2;
	nPos -= nMax;
	BYTE r = r2 - (r2 - r1) * nPos / nMax;
	BYTE g = g2 - (g2 - g1) * nPos / nMax;
	BYTE b = b2 - (b2 - b1) * nPos / nMax;

	return RGB(r, g, b);
}


// CMyProgressCtrl message handlers

void CMyProgressCtrl::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	// TODO: Add your message handler code here
	CRect cRect;
	GetClientRect(cRect);

	CString sPos;
	int nLower, nUpper;
	CRect rcRange = cRect;

	GetRange(nLower, nUpper);
	if (nUpper - nLower > 0)
	{
		sPos.Format(_T("%d%%"), 100 * GetPos() / (nUpper - nLower));
		rcRange.right = (int)(rcRange.left + rcRange.Width() * (double)GetPos() / (nUpper - nLower));
	}
	else
	{
		sPos = "0%";
		dc.FillSolidRect(cRect, GetSysColor(COLOR_3DFACE));
		dc.Draw3dRect(cRect, RGB(0, 0, 0), RGB(0, 0, 0));
		dc.SetBkMode(TRANSPARENT);
		dc.DrawText(sPos, cRect, DT_CENTER);
		return;
	}

	CDC memDC, memDC2;
	CBitmap bmp, bmp2;

	bmp.CreateCompatibleBitmap(&dc, cRect.Width(), cRect.Height());
	memDC.CreateCompatibleDC(&dc);
	memDC.SelectObject(&bmp);

	bmp2.CreateCompatibleBitmap(&dc, cRect.Width(), cRect.Height());
	memDC2.CreateCompatibleDC(&dc);
	memDC2.SelectObject(&bmp2);

	memDC2.FillSolidRect(cRect, RGB(255, 0, 0));	//底色用RGB(255,0,0)
	memDC2.FillSolidRect(rcRange, RGB(0, 255, 255));	//已完成的进度用RGB(0,255,255)填充

	memDC.FillSolidRect(cRect, RGB(0, 0, 0));
	memDC.SetBkMode(TRANSPARENT);
	memDC.SetTextColor(RGB(255, 0, 0));
	memDC.DrawText(sPos, cRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);		//memDC中现在是黑底红字

	//SRCPAINT是OR操作, 所以现在memDC2中, 落在已完成进度区域内的字是白色(0|255, 255|255, 255|255), 落在未完成区域内的字和底色融为一体了(255|255, 0|0, 0|0)。
	memDC2.BitBlt(0, 0, cRect.Width(), cRect.Height(), &memDC, 0, 0, SRCPAINT);

	memDC.FillSolidRect(cRect, RGB(255, 0, 0));

	//把已完成部分的底色RGB(0,255,255)设为透明色，现在memDC中只剩下红底白字了。
	//memDC.TransparentBlt(0,0,cRect.Width(),cRect.Height(),&memDC2,0,0,cRect.Width(),cRect.Height(),RGB(0,255,255));
	TransparentBlt(memDC.m_hDC, 0, 0, cRect.Width(), cRect.Height(), memDC2.m_hDC, 0, 0, cRect.Width(), cRect.Height(), RGB(0, 255, 255));

	memDC2.FillSolidRect(cRect, GetSysColor(COLOR_3DFACE));
	memDC2.SetBkMode(TRANSPARENT);
	memDC2.SetTextColor(RGB(0, 0, 0));
	memDC2.DrawText(sPos, cRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);	//用黑色绘制文字

	//绘制已完成进度的进度条，完成后覆盖了已完成区域内的黑色文字
	for (int i = rcRange.top; i < rcRange.bottom; i++)
	{
		CPen pen, * pOldPen;
		pen.CreatePen(PS_SOLID, 1, GetColor(i - rcRange.top, rcRange.Height()));
		pOldPen = memDC2.SelectObject(&pen);

		memDC2.MoveTo(rcRange.left, i);
		memDC2.LineTo(rcRange.right, i);

		memDC2.SelectObject(pOldPen);
	}

	//把memDC中的白字部分叠加到memDC2中，完成进度条绘制。	
	//memDC2.TransparentBlt(0,0,cRect.Width(),cRect.Height(),&memDC,0,0,cRect.Width(),cRect.Height(),RGB(255,0,0));
	TransparentBlt(memDC2.m_hDC, 0, 0, cRect.Width(), cRect.Height(), memDC.m_hDC, 0, 0, cRect.Width(), cRect.Height(), RGB(255, 0, 0));

	memDC2.Draw3dRect(0, 0, cRect.Width(), cRect.Height(), RGB(0, 0, 0), RGB(0, 0, 0));
	dc.BitBlt(0, 0, cRect.Width(), cRect.Height(), &memDC2, 0, 0, SRCCOPY);

	memDC2.DeleteDC();
	memDC.DeleteDC();
	bmp.DeleteObject();
	bmp2.DeleteObject();

	// Do not call CProgressCtrl::OnPaint() for painting messages
}


