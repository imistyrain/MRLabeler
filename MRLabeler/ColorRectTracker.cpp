#include "stdafx.h"
#include "ColorRectTracker.h"


CColorRectTracker::CColorRectTracker()
{
}


CColorRectTracker::~CColorRectTracker()
{
}
void CColorRectTracker::Draw(CDC* pDC, CPen *pPen)const 
{
	if ((m_nStyle & dottedLine) != 0)
	{
		VERIFY(pDC->SaveDC() != 0);
		pDC->SetMapMode(MM_TEXT);
		pDC->SetViewportOrg(0, 0);
		pDC->SetWindowOrg(0, 0);

		CRect rect = m_rect;
		rect.NormalizeRect();

		CPen *pOldPen = NULL;
		CBrush *pOldBrush = NULL;
		int nOldROP;

		pOldPen = (CPen*)pDC->SelectObject(pPen);
		pDC->SetBkMode(TRANSPARENT);
		pDC->SetTextColor(RGB(0, 0, 255));
		pDC->DrawText(m_strName, rect, NULL);
		pOldBrush = (CBrush*)pDC->SelectStockObject(NULL_BRUSH);
		nOldROP = pDC->SetROP2(R2_COPYPEN);

		rect.InflateRect(+1, +1);
		pDC->Rectangle(rect.left, rect.top, rect.right, rect.bottom);
		pDC->SetROP2(nOldROP);
		
		if ((m_nStyle & (resizeInside | resizeOutside)) != 0)
		{
			UINT mask = GetHandleMask();
			for (int i = 0; i < 8; ++i)
				if (mask & (1 << i))
				{
					GetHandleRect((TrackerHit)i, &rect);
					LOGPEN logpen;
					pPen->GetLogPen(&logpen);
//					pDC->FillSolidRect(rect, logpen.lopnColor);
				}
		}
	}
	else
		CRectTracker::Draw(pDC);
}