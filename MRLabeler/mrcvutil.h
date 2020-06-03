#ifndef _MROPENCVUTIL_H_
#define _MROPENCVUTIL_H_
#include "opencv2/opencv.hpp"
#include "atlimage.h"
static BOOL Mat2CImage(const cv::Mat& src_img, CImage& dst_img)
{
	if (!src_img.data)
	{
		return FALSE;
	}
	int width = src_img.cols;
	int height = src_img.rows;
	int channels = src_img.channels();
	int src_type = src_img.type();

	dst_img.Destroy();

	switch (src_type)
	{
	case CV_8UC1:
	{
		dst_img.Create(width, -1 * height, 8 * channels);
		unsigned char* dst_data = static_cast<unsigned char*>(dst_img.GetBits());
		int step_size = dst_img.GetPitch();
		unsigned char* src_data = nullptr;
		for (int i = 0; i < height; i++)
		{
			src_data = const_cast<unsigned char*>(src_img.ptr<unsigned char>(i));    
			for (int j = 0; j < width; j++)
			{
				if (step_size > 0)
				{
					*(dst_data + step_size*i + j) = *src_data++;
				}  
				else
				{
					*(dst_data + step_size*i - j) = *src_data++;
				}
			}
		}
		break;
	}
	case CV_8UC3:
	{
		dst_img.Create(width, height, 8 * channels);
		unsigned char* dst_data = static_cast<unsigned char*>(dst_img.GetBits());
		int step_size = dst_img.GetPitch();
		unsigned char* src_data = nullptr;
		for (int i = 0; i < height; i++)
		{
			src_data = const_cast<unsigned char*>(src_img.ptr<unsigned char>(i));    
			for (int j = 0; j < width; j++)
			{
				for (int k = 0; k < 3; k++)
				{
					*(dst_data + step_size*i + j * 3 + k) = src_data[3 * j + k];
				}
			}
		}
		break;
	}
	default:
		break;
	}
	return TRUE;
}


//功能：把Mat绘制到pWnd所代表的窗体上，使用方法如下所示:
//DrawMatToWnd(GetDlgItem(IDC_PIC), img);
static void DrawMat2CWnd(CWnd* pWnd, cv::Mat &img, CRect *Roi = NULL)
{
	CImage imgDst;
	Mat2CImage(img,imgDst);
	CRect drect;
	if (Roi == NULL)
		pWnd->GetClientRect(drect);
	else
		drect = *Roi;
	CDC* dc=pWnd->GetDC();
	imgDst.Draw(dc->GetSafeHdc(), drect);
	pWnd->ReleaseDC(dc);
}

static void DrawMat2Wnd(const HWND &hWnd, cv::Mat &img, CRect *Roi = NULL)
{
	CWnd*pWnd = CWnd::FromHandle(hWnd);
	DrawMat2CWnd(pWnd, img);
}

static void BitMat2Wnd(CWnd* wnd, cv::Mat &img, CRect *Roi=NULL)
{
	if (img.empty())
		return;
	CDC *cdc = wnd->GetDC();
	CDC MemDC;
	CBitmap MemBitmap;
	CRect rect, drect;
	wnd->GetClientRect(rect);
	Gdiplus::Bitmap bitmap(img.cols, img.rows, img.cols * img.channels(), PixelFormat24bppRGB, (BYTE*)img.data);
	if (Roi == NULL)
		drect = rect;
	else
		drect = *Roi;
	MemDC.CreateCompatibleDC(cdc);
	MemBitmap.CreateCompatibleBitmap(cdc, rect.Width(), rect.Height());
	CBitmap *pOldBit = MemDC.SelectObject(&MemBitmap);
	MemDC.FillSolidRect(0, 0, rect.Width(), rect.Height(), RGB(255, 255, 255));
	Gdiplus::Graphics g(MemDC.m_hDC);
	Gdiplus::Image *ii = &bitmap;
	g.DrawImage(ii, Gdiplus::Rect(0, 0, drect.Width(), drect.Height()));
	g.ReleaseHDC(MemDC.m_hDC);
	cdc->BitBlt(0, 0, drect.Width(), drect.Height(), &MemDC, 0, 0, SRCCOPY);
	MemBitmap.DeleteObject();
	MemDC.DeleteDC();
	wnd->ReleaseDC(cdc);
}

static void DrawMat2DuiWnd(CWnd* pWnd, cv::Mat &img, CRect *Roi = NULL)
{
	if (img.empty())
		return;
	CRect drect;
	pWnd->GetClientRect(drect);
	CClientDC dc(pWnd);
	HDC hDC = dc.GetSafeHdc();
	BYTE *bitBuffer = NULL;
	BITMAPINFO *bitMapinfo = NULL;
	int ichannels = img.channels();
	if (ichannels == 1)
	{
		bitBuffer = new BYTE[40 + 4 * 256];
	}
	else if (ichannels == 3)
	{
		bitBuffer = new BYTE[sizeof(BITMAPINFO)];
	}
	else
	{
		return;
	}
	if (bitBuffer == NULL)
	{
		return;
	}
	bitMapinfo = (BITMAPINFO *)bitBuffer;
	bitMapinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitMapinfo->bmiHeader.biHeight = -img.rows; //如果高度为正的，位图的起始位置在左下角。如果高度为负，起始位置在左上角。 
	bitMapinfo->bmiHeader.biWidth = img.cols;
	bitMapinfo->bmiHeader.biPlanes = 1; // 目标设备的级别，必须为1 
	bitMapinfo->bmiHeader.biBitCount = ichannels * 8; // 每个像素所需的位数，必须是1(双色), 4(16色)，8(256色)或24(真彩色)之一 
	bitMapinfo->bmiHeader.biCompression = BI_RGB; //位图压缩类型，必须是 0(不压缩), 1(BI_RLE8压缩类型)或2(BI_RLE4压缩类型)之一 
	bitMapinfo->bmiHeader.biSizeImage = 0; // 位图的大小，以字节为单位 
	bitMapinfo->bmiHeader.biXPelsPerMeter = 0; // 位图水平分辨率，每米像素数 
	bitMapinfo->bmiHeader.biYPelsPerMeter = 0; // 位图垂直分辨率，每米像素数 
	bitMapinfo->bmiHeader.biClrUsed = 0; // 位图实际使用的颜色表中的颜色数 
	bitMapinfo->bmiHeader.biClrImportant = 0; // 位图显示过程中重要的颜色数
	if (ichannels == 1)
	{
		for (int i = 0; i < 256; i++)
		{ //颜色的取值范围 (0-255) 
			bitMapinfo->bmiColors[i].rgbBlue = bitMapinfo->bmiColors[i].rgbGreen = bitMapinfo->bmiColors[i].rgbRed = (BYTE)i;
		}

		bitMapinfo->bmiHeader.biClrUsed = 256; // 位图实际使用的颜色表中的颜色数 
	}
	SetStretchBltMode(hDC, COLORONCOLOR);
	StretchDIBits(hDC,0,0,drect.right,drect.bottom,0,0,img.cols,img.rows,img.data,bitMapinfo,DIB_RGB_COLORS,SRCCOPY);
	delete[]bitBuffer;
}
#endif