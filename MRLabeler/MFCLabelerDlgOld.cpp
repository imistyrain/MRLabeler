
// MFCLabelerDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MFCLabeler.h"
#include "MFCLabelerDlg.h"
#include "afxdialogex.h"
#include "mrdir.h"
#include "mrutil.h"
#include "mrcvutil.h"
#include "mrutf.h"
#include "mropencv.h"

#include <sstream>
using namespace  std;
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMFCLabelerDlg 对话框

CMFCLabelerDlg::CMFCLabelerDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMFCLabelerDlg::IDD, pParent)
	, m_imgindex(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFCLabelerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_imgindex);
	DDX_Control(pDX, IDC_COMBO_CLASSNAMES, m_comboxclassnames);
}

BEGIN_MESSAGE_MAP(CMFCLabelerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_OPEN, &CMFCLabelerDlg::OnBnClickedButtonOpen)
	ON_BN_CLICKED(IDC_BUTTON_PREV, &CMFCLabelerDlg::OnBnClickedButtonPrev)
	ON_BN_CLICKED(IDC_BUTTON_NEXT, &CMFCLabelerDlg::OnBnClickedButtonNext)
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_BN_CLICKED(IDC_BUTTON_ADDRECTANGLE, &CMFCLabelerDlg::OnBnClickedButtonAddrectangle)
	ON_WM_SETCURSOR()
	ON_CBN_SELCHANGE(IDC_COMBO_CLASSNAMES, &CMFCLabelerDlg::OnCbnSelchangeComboClassnames)
END_MESSAGE_MAP()


// CMFCLabelerDlg 消息处理程序

BOOL CMFCLabelerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
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
	GetDlgItem(IDC_PIC)->GetWindowRect(&m_Picrect);
	ScreenToClient(&m_Picrect);
	m_nLastWidth = m_Picrect.Width();
	m_nLastHeight = m_Picrect.Height();
	// TODO:  在此添加额外的初始化代码
	for (int i = 0; i < voc.classes.size(); i++)
	{
		auto str = ANSIToUnicode(voc.classes[i].c_str());
		m_comboxclassnames.AddString(str);
	}
	m_comboxclassnames.SetCurSel(0);
	m_currentselectedclassname = ANSIToUnicode(voc.classes[0].c_str());
	m_imgdir=voc.rootdir + "/" + voc.imagedir;
	getAllFilesinDir(m_imgdir, m_files);
	ShowImageOfIndex();
	return TRUE;
}

void CMFCLabelerDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CMFCLabelerDlg::OnPaint()
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
		if (!m_img.empty())
		{
			DrawMat2Wnd(GetDlgItem(IDC_PIC), m_showimg);
		}
		UpdateTrackers();
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CMFCLabelerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMFCLabelerDlg::UpdateTrackers()
{
	CPaintDC dc(this);
	for (int i = 0; i < m_trackers.size(); i++)
	{
		m_trackers[i].Draw(&dc);
	}
}

void CMFCLabelerDlg::ShowImageOfIndex()
{
	if (m_files.size() > 0)
	{
		string filepath = m_imgdir + "/" + m_files[m_imgindex];
		SetWindowText(ANSIToUnicode(m_files[m_imgindex].c_str()));
		m_img = cv::imread(filepath);
		if (m_img.data)
		{
			m_trackers.clear();
			string annotationfilepath = voc.rootdir + "/" + voc.annotationdir + "/" + m_files[m_imgindex].substr(0, m_files[m_imgindex].length() - 4) + ".xml";
			LoadAnnotationFile(annotationfilepath);
			cv::resize(m_img, m_showimg, cv::Size(m_Picrect.Width(), m_Picrect.Height()));
			UpdateView();
		}
	}
}

void CMFCLabelerDlg::OnBnClickedButtonOpen()
{
	TCHAR szPath[MAX_PATH];
	ZeroMemory(szPath, sizeof(szPath));
	BROWSEINFO bi;
	bi.hwndOwner = m_hWnd;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = szPath;
	bi.lpszTitle = _T("请选择要标记的文件夹");
	bi.ulFlags = 0;
	bi.lpfn = NULL;
	bi.lParam = 0;
	bi.iImage = 0;
	LPITEMIDLIST lp = SHBrowseForFolder(&bi);
	if (lp && SHGetPathFromIDList(lp, szPath))
	{
		m_imgdir = UnicodeToANSI(szPath);
		getAllFilesinDir(m_imgdir, m_files,"*.jpg");
		m_imgindex = 0;
		ShowImageOfIndex();
	}
}

void CMFCLabelerDlg::OnBnClickedButtonPrev()
{
	// TODO:  在此添加控件通知处理程序代码
	if (m_files.size() > 0)
	{
		m_imgindex = (m_imgindex - 1 + m_files.size()) % m_files.size();
		ShowImageOfIndex();
	}
}

void CMFCLabelerDlg::OnBnClickedButtonNext()
{
	// TODO:  在此添加控件通知处理程序代码
	if (m_files.size() > 0)
	{
		m_imgindex = (m_imgindex + 1) % m_files.size();
		ShowImageOfIndex();
	}
}

void CMFCLabelerDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	if (!m_bDragMode)
	{
		CColorRectTracker tracker;
		tracker.m_strName = m_currentselectedclassname;
		tracker.m_rect.SetRect(0, 0, 0, 0);
		tracker.m_nStyle = CRectTracker::dottedLine | CRectTracker::resizeInside;
		tracker.TrackRubberBand(this, point, TRUE);
		tracker.m_rect.NormalizeRect();
		CClientDC dc(this);
		tracker.Draw(&dc);
		m_trackers.push_back(tracker);
		m_bDragMode = true;
	}
	else
	{
		for (int i = m_trackers.size() - 1; i >= 0; i--)
		{
			int nRetCode = m_trackers[i].HitTest(point);
			if (nRetCode >= 0)
			{
				m_trackers[i].Track(this, point, TRUE);
				CColorRectTracker tracker = m_trackers[i];
				m_trackers.erase(m_trackers.begin() + i);
				m_trackers.push_back(tracker);
				UpdateView();
				return;
			}
		}
	}
// 	m_bDrawing = true;
// 	m_ptRelativeBegin = cv::Point2f(point.x*1.0 / m_Picrect.Width(), point.y *1.0 / m_Picrect.Height());
// 	m_ptRelativeEnd = m_ptRelativeBegin;
//	string pos = "X,Y:" + int2string((int)(m_ptRelativeEnd.x*m_Picrect.Width())) + "," + int2string((int)(m_ptRelativeEnd.y*m_Picrect.Height()));
//	GetDlgItem(IDC_POS)->SetWindowText(ANSIToUnicode(pos).c_str());
	CDialogEx::OnLButtonDown(nFlags, point);
}

void CMFCLabelerDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if (PtInRect(m_Picrect, point))
	{
		string pos = "X,Y:" + int2string(point.x) + "," + int2string(point.y);
		GetDlgItem(IDC_POS)->SetWindowText(ANSIToUnicode(pos).c_str());
	}
// 	if (m_bDrawing)
// 	{
// 		m_ptRelativeEnd = cv::Point2f(point.x*1.0 / m_Picrect.Width(), point.y *1.0 / m_Picrect.Height());
// //		string pos = "X,Y:" + int2string((int)(m_ptRelativeEnd.x*m_Picrect.Width())) + "," + int2string((int)(m_ptRelativeEnd.y*m_Picrect.Height()));
// //		GetDlgItem(IDC_POS)->SetWindowText(ANSIToUnicode(pos).c_str());
// //		Invalidate(FALSE);
// 	}
	CDialogEx::OnMouseMove(nFlags, point);
}

void CMFCLabelerDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
//	m_bDrawing = false;
//	m_ptRelativeEnd = cv::Point2f(point.x *1.0 / m_Picrect.Width(), point.y*1.0 / m_Picrect.Height());
//	string pos = "X,Y:" + int2string((int)(m_ptRelativeEnd.x*m_Picrect.Width())) + "," + int2string((int)(m_ptRelativeEnd.y*m_Picrect.Height()));
//	GetDlgItem(IDC_POS)->SetWindowText(ANSIToUnicode(pos).c_str());
//	Invalidate(FALSE);
	CDialogEx::OnLButtonUp(nFlags, point);
}

void CMFCLabelerDlg::ResizeBottomButton(int nID)
{
	CRect rect;
	GetDlgItem(nID)->GetWindowRect(&rect);
	ScreenToClient(&rect);
	rect.top = dlg_rect.bottom -35;
	rect.bottom = dlg_rect.bottom-5;
	GetDlgItem(nID)->MoveWindow(rect);
}
void CMFCLabelerDlg::ResizeBottomStatic(int nID)
{
	CRect rect;
	GetDlgItem(nID)->GetWindowRect(&rect);
	ScreenToClient(&rect);
	rect.top = dlg_rect.bottom - 30;
	rect.bottom = dlg_rect.bottom-10;
	GetDlgItem(nID)->MoveWindow(rect);
}
void CMFCLabelerDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
	if (GetDlgItem(IDC_PIC) == NULL)return;
 	GetClientRect(&dlg_rect);
	ResizeBottomButton(IDC_BUTTON_OPEN);
	ResizeBottomButton(IDC_BUTTON_ADDRECTANGLE);
	ResizeBottomStatic(IDC_COMBO_CLASSNAMES);
	ResizeBottomStatic(IDC_EDIT1);
	ResizeBottomButton(IDC_BUTTON_PREV);
	ResizeBottomButton(IDC_BUTTON_NEXT);
	ResizeBottomStatic(IDC_POS);

	GetDlgItem(IDC_PIC)->GetWindowRect(&m_Picrect);
	ScreenToClient(&m_Picrect);
	m_Picrect.right = dlg_rect.right;
	m_Picrect.bottom = dlg_rect.bottom-60;
 	GetDlgItem(IDC_PIC)->MoveWindow(m_Picrect);
	if (m_img.data)
	{
		resize(m_img, m_showimg, cv::Size(m_Picrect.right - m_Picrect.left, m_Picrect.bottom - m_Picrect.top));
	}
// 	for (int i = 0; i < m_trackers.size();i++)
// 	{
// 		CColorRectTracker tracker = m_trackers[i];
// 		CRect rect;
// 		tracker.GetTrueRect(rect);
// 		rect.left = rect.left*1.0 / m_nLastWidth*m_Picrect.Width();
// 		rect.right = rect.right*1.0 / m_nLastWidth*m_Picrect.Width();
// 		rect.top = rect.top*1.0 / m_nLastHeight*m_Picrect.Height();
// 		rect.bottom = rect.bottom*1.0 / m_nLastHeight*m_Picrect.Height();
// 		tracker.m_rect.SetRect(rect.left,rect.top,rect.right,rect.bottom);
// //		Invalidate(TRUE);
// //		tracker.m_rect.NormalizeRect();
// 	}
	m_nLastWidth = m_Picrect.Width();
	m_nLastHeight = m_Picrect.Height();
	UpdateView();
}


void CMFCLabelerDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	lpMMI->ptMinTrackSize.x = 640;
	lpMMI->ptMinTrackSize.y = 480;
	lpMMI->ptMaxTrackSize.x = 1600;
	lpMMI->ptMaxTrackSize.y = 900;
	CDialogEx::OnGetMinMaxInfo(lpMMI);
}

BOOL CMFCLabelerDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO:  在此添加专用代码和/或调用基类
	if (pMsg->message == WM_KEYUP&&pMsg->wParam == VK_DELETE)
	{
		if (m_trackers.size() > 0)
		{
			m_trackers.pop_back();
			UpdateView();
		}
		if (m_trackers.size() == 0)
			m_bDragMode = false;
		return 0;
	}
	if (WM_KEYDOWN == pMsg->message && VK_RETURN == pMsg->wParam)
	{
		if (GetFocus() == GetDlgItem(IDC_EDIT1))
		{
			CString str;
			GetDlgItem(IDC_EDIT1)->GetWindowText(str);
			m_imgindex = _ttoi(str);
			ShowImageOfIndex();
			return TRUE;
		}
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}

void CMFCLabelerDlg::UpdateView()
{
	if (m_img.data)
		Invalidate(FALSE);
	else
		Invalidate();
}

void CMFCLabelerDlg::OnBnClickedButtonAddrectangle()
{
	// TODO:  在此添加控件通知处理程序代码
	m_bDragMode = false;
}


BOOL CMFCLabelerDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	if ((pWnd == this))
	{
		for (int i = 0; i < m_trackers.size(); i++)
		{
			if (m_trackers[i].SetCursor(this, nHitTest))
				return TRUE;
		}
	}
	return CDialogEx::OnSetCursor(pWnd, nHitTest, message);
}


void CMFCLabelerDlg::OnCbnSelchangeComboClassnames()
{
	// TODO:  在此添加控件通知处理程序代码
	m_comboxclassnames.GetLBText(m_comboxclassnames.GetCurSel(), m_currentselectedclassname);
}

void CMFCLabelerDlg::LoadAnnotationFile(const string xmlannopath)
{
	AnnotationFile af;
	if (af.load_file(xmlannopath))
	{
		m_trackers.clear();
		for (int i = 0; i < af.objects.size(); i++)
		{
			auto object = af.objects[i];
			int xmin = object.xmin *1.0 / af.width*m_Picrect.Width();
			int ymin = object.ymin *1.0 / af.height*m_Picrect.Height();
			int xmax = object.xmax *1.0 / af.width*m_Picrect.Width();
			int ymax = object.ymax *1.0 / af.height*m_Picrect.Height();
			CColorRectTracker tracker;
			tracker.m_strName = object.name.c_str();
			tracker.m_rect.SetRect(xmin, ymin, xmax, ymax);
			tracker.m_nStyle = CRectTracker::dottedLine | CRectTracker::resizeInside;
			tracker.m_rect.NormalizeRect();
			m_trackers.push_back(tracker);
			m_bDragMode = true;
		}
	}
}