#pragma once
#include "afxext.h"
class CColorRectTracker:public CRectTracker
{
public:
	CString m_strName;
	CColorRectTracker();
	~CColorRectTracker();
	void Draw(CDC* pDC, CPen *pPen = &CPen(PS_SOLID, 1, RGB(0, 255, 0)))const;
};

