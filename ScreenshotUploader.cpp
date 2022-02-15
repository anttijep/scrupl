#include "ScreenshotUploader.h"
#include <iostream>
#include <vector>
#include <memory>
#include <gdiplus.h>
#include <gdiplusbase.h>

#include "lodepng.h"

#define GET_X(x) (x & 0xffff)
#define GET_Y(y) ((y >> 16) & 0xffff)


ScreenshotUploader::ScreenshotUploader()
{
	bmp.bmBits = nullptr;
}

ScreenshotUploader::~ScreenshotUploader()
{
}

bool CompareCorners(int x1, int y1, int x2, int y2, int threshold)
{
	return std::abs(x1 - x2) < threshold && std::abs(y1 - y2) < threshold;
}

bool ScreenshotUploader::KeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case (VK_F8):
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	default:
		return false;
	}
	return true;
}

bool ScreenshotUploader::HandleResize(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	if (!bitmapIsValid)
		return false;
	int newX = LOWORD(lParam),
		newY = HIWORD(lParam);

	SCROLLINFO si;
	si.cbSize = sizeof(si);

	int xmax = max(bmp.bmWidth - newX, 0);
	if (scrollX > xmax)
		scrollX = xmax;
	si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
	si.nMin = 0;
	si.nMax = bmp.bmWidth;
	si.nPage = newX;
	si.nPos = scrollX;
	SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);

	int ymax = max(bmp.bmHeight - newY, 0);
	if (scrollY > ymax)
		scrollY = ymax;

	si.nMin = 0;
	si.nMax = bmp.bmHeight;
	si.nPage = newY;
	si.nPos = scrollY;
	SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
	ValidateWindow(hWnd);

	InvalidateRect(hWnd, NULL, TRUE);
	return true;
}

bool ScreenshotUploader::HandleVScroll(HWND hWnd, WPARAM wParam, LPARAM lParam, short fixed)
{
	if (!bitmapIsValid)
		return false;
	int yNewPos;
	if (fixed != 0x7fff) {
		yNewPos = scrollY - fixed;
	}
	else {
		switch (LOWORD(wParam))
		{
		case SB_PAGEUP:
			yNewPos = scrollY - 50;
			break;
		case SB_PAGEDOWN:
			yNewPos = scrollY + 50;
			break;
		case SB_LINEUP:
			yNewPos = scrollY - 5;
			break;
		case SB_LINEDOWN:
			yNewPos = scrollY + 5;
			break;
		case SB_THUMBPOSITION:
			yNewPos = HIWORD(wParam);
			break;
		case SB_THUMBTRACK:
			yNewPos = HIWORD(wParam);
			break;
		default:
			yNewPos = scrollY;
		}
	}
	if (yNewPos < 0)
		yNewPos = 0;
	RECT rect;
	GetClientRect(hWnd, &rect);
	yNewPos = min(max(bmp.bmHeight - rect.bottom, 0), yNewPos);
	int yDelta = yNewPos - scrollY;
	scrollY = yNewPos;

	ScrollWindowEx(hWnd, 0, -yDelta, NULL, NULL, NULL, NULL, SW_INVALIDATE);
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_POS;
	si.nPos = scrollY;
	SetScrollInfo(hWnd, SB_VERT, &si, TRUE);

	return true;
}

bool ScreenshotUploader::HandleHScroll(HWND hWnd, WPARAM wParam, LPARAM lParam, short fixed)
{
	if (!bitmapIsValid)
		return false;
	int xNewPos;
	if (fixed != 0x7fff) {
		xNewPos = scrollX - fixed;
	}
	else {
		switch (LOWORD(wParam))
		{
		case SB_PAGEUP:
			xNewPos = scrollX - 50;
			break;
		case SB_PAGEDOWN:
			xNewPos = scrollX + 50;
			break;
		case SB_LINEUP:
			xNewPos = scrollX - 5;
			break;
		case SB_LINEDOWN:
			xNewPos = scrollX + 5;
			break;
		case SB_THUMBPOSITION:
			xNewPos = HIWORD(wParam);
			break;
		case SB_THUMBTRACK:
			xNewPos = HIWORD(wParam);
			break;
		default:
			xNewPos = scrollX;
		}
	}
	RECT rect;
	GetClientRect(hWnd, &rect);

	if (xNewPos < 0)
		xNewPos = 0;
	xNewPos = min(max(bmp.bmWidth - rect.right, 0), xNewPos);
	if (xNewPos == scrollX)
		return true;
	int xDelta = xNewPos - scrollX;

	scrollX = xNewPos;

	ScrollWindowEx(hWnd, -xDelta, 0, NULL, NULL, NULL, NULL, SW_INVALIDATE);
	UpdateWindow(hWnd);

	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_POS;
	si.nPos = scrollX;
	SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);

	return true;
}


bool ScreenshotUploader::HandleMouse(UINT message, HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_LBUTTONDOWN:
	{
		int clickX = GET_X(lParam) + scrollX - doffsetX;
		int clickY = GET_Y(lParam) + scrollY - doffsetY;
		if (selection.isSelected)
		{
			int lx = min(selection.bx, selection.ex);
			int rx = max(selection.bx, selection.ex);
			int ty = min(selection.by, selection.ey);
			int by = max(selection.by, selection.ey);
			const static int THRESHOLD = 5;

			if (CompareCorners(lx, ty, clickX, clickY, THRESHOLD))
			{
				selection.bx = rx;
				selection.by = by;
				selection.ex = lx;
				selection.ey = ty;
				selection.isSelecting = true;
			}
			else if (CompareCorners(lx, by, clickX, clickY, THRESHOLD))
			{
				selection.bx = rx;
				selection.by = ty;
				selection.ex = lx;
				selection.ey = by;
				selection.isSelecting = true;
			}
			else if (CompareCorners(rx, ty, clickX, clickY, THRESHOLD))
			{
				selection.bx = lx;
				selection.by = by;
				selection.ex = rx;
				selection.ey = ty;
				selection.isSelecting = true;
			}
			else if (CompareCorners(rx, by, clickX, clickY, THRESHOLD))
			{
				selection.bx = lx;
				selection.by = ty;
				selection.ex = rx;
				selection.ey = by;
				selection.isSelecting = true;
			}
		}
		if (!selection.isSelecting)
		{
			selection.bx = clickX;
			selection.by = clickY;
			selection.isSelected = false;
			selection.isSelecting = true;
		}
		InvalidateRect(hWnd, NULL, FALSE);
	}
	break;
	case WM_LBUTTONUP:
	{
		int oldx = selection.ex, oldy = selection.ey;
		selection.ex = GET_X(lParam) + scrollX - doffsetX;
		selection.ey = GET_Y(lParam) + scrollY - doffsetY;
		if (selection.bx == selection.ex || selection.by == selection.ey)
			selection.isSelected = false;
		else
			selection.isSelected = true;
		selection.isSelecting = false;
		RECT r;
		r.left = min(min(oldx, selection.ex), selection.bx) - scrollX - (LONG)penwidth + doffsetX;
		r.top = min(min(oldy, selection.ey), selection.by) - scrollY - (LONG)penwidth + doffsetY;
		r.right = max(max(oldx, selection.ex), selection.bx) - scrollX + (LONG)penwidth + doffsetX;
		r.bottom = max(max(oldy, selection.ey), selection.by) - scrollY + (LONG)penwidth + doffsetY;
		InvalidateRect(hWnd, &r, FALSE);
	//	InvalidateRect(hWnd, NULL, FALSE);
	}
	break;
	case WM_MOUSEMOVE:
	{
		if (selection.isSelecting == true)
		{
			int oldx = selection.ex, oldy = selection.ey;
			selection.ex = GET_X(lParam) + scrollX - doffsetX;
			selection.ey = GET_Y(lParam) + scrollY - doffsetY;
			if (selection.bx == selection.ex || selection.by == selection.ey)
				selection.isSelected = false;
			else
				selection.isSelected = true;
			RECT r;
			r.left = min(min(oldx, selection.ex), selection.bx) - scrollX - (LONG)penwidth + doffsetX;
			r.top = min(min(oldy, selection.ey), selection.by) - scrollY - (LONG)penwidth + doffsetY;
			r.right = max(max(oldx, selection.ex), selection.bx) - scrollX + (LONG)penwidth + doffsetX;
			r.bottom = max(max(oldy, selection.ey), selection.by) - scrollY + (LONG)penwidth + doffsetY;
			InvalidateRect(hWnd, &r, FALSE);
		}
	}
	break;
	default:
		return false;
	}
	return true;
}

void ScreenshotUploader::ValidateWindow(HWND hWnd)
{
	bool redraw = false;
	auto style = GetWindowLongPtr(hWnd, GWL_STYLE);

	RECT screenRect;
	GetClientRect(hWnd, &screenRect);


	if (bmp.bmWidth > screenRect.right) {
		if (!(style & WS_HSCROLL)) {
			SetWindowLongPtr(hWnd, GWL_STYLE, style | WS_HSCROLL);
			redraw = true;
		}
		doffsetX = 0;
	}
	else {
		if (style & WS_HSCROLL) {
			SetWindowLongPtr(hWnd, GWL_STYLE, style & ~WS_HSCROLL);
			redraw = true;
		}

		doffsetX = (screenRect.right - bmp.bmWidth) / 2;

	}
	if (bmp.bmHeight > screenRect.bottom) {
		if (!(style & WS_VSCROLL)) {
			SetWindowLongPtr(hWnd, GWL_STYLE, style | WS_VSCROLL);
		}
		doffsetY = 0;
	}
	else {
		if (style & WS_VSCROLL) {
			SetWindowLongPtr(hWnd, GWL_STYLE, style & ~WS_VSCROLL);
		}
		doffsetY = (screenRect.bottom - bmp.bmHeight) / 2;
	}

	SCROLLINFO si;
	si.cbSize = sizeof(si);
	int xmax = max(bmp.bmWidth - screenRect.right, 0);
	if (scrollX > xmax)
		scrollX = xmax;
	si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
	si.nMin = 0;
	si.nMax = bmp.bmWidth;
	si.nPage = screenRect.right;
	si.nPos = scrollX;
	SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);

	int ymax = max(bmp.bmHeight - screenRect.bottom, 0);
	if (scrollY > ymax)
		scrollY = ymax;

	si.nMin = 0;
	si.nMax = bmp.bmHeight;
	si.nPage = screenRect.bottom;
	si.nPos = scrollY;
	SetScrollInfo(hWnd, SB_VERT, &si, TRUE);

	SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOSIZE | SWP_FRAMECHANGED);
}

unsigned int ScreenshotUploader::GetPNG(std::vector<unsigned char>& pngOut)
{
	if (!bitmapIsValid)
		return 3;
	std::vector<unsigned char> img;

	unsigned numchannels = bmp.bmBitsPixel / 8;

	unsigned beginx = 0;
	unsigned beginy = 0;
	LONG width = bmp.bmWidth;
	LONG height = bmp.bmHeight;
	if (selection.isSelected)
	{
		int lx = min(selection.bx, selection.ex);
		int rx = max(selection.bx, selection.ex);
		int ty = min(selection.by, selection.ey);
		int by = max(selection.by, selection.ey);

		if (lx >= bmp.bmWidth || ty >= bmp.bmHeight)
			return 4;
		beginx = max(lx, 0);
		beginy = max(ty, 0);
		width = min(rx - beginx, bmp.bmWidth - beginx);
		height = min(by - beginy, bmp.bmHeight - beginy);
	}
	img.resize(((width * bmp.bmBitsPixel + 31) / 32) * 4 * height);

	for (LONG y = 0; y < height; ++y) {
		for (LONG x = 0; x < width; ++x) {
			unsigned bmpos = (bmp.bmHeight - y - beginy - 1) * bmp.bmWidthBytes + numchannels * (x + beginx);
			unsigned pngpos = 4 * y * width + 4 * x;
			if (numchannels == 4) {
				img[pngpos] = bitmapData[bmpos + 2];
				img[pngpos + 1] = bitmapData[bmpos + 1];
				img[pngpos + 2] = bitmapData[bmpos + 0];
				img[pngpos + 3] = bitmapData[bmpos + 3];
			}
			else {
				img[pngpos] = bitmapData[bmpos + 2];
				img[pngpos + 1] = bitmapData[bmpos + 1];
				img[pngpos + 2] = bitmapData[bmpos + 0];
				img[pngpos + 3] = 255;
			}
		}
	}

	return lodepng::encode(pngOut, img, width, height);
}



void ScreenshotUploader::CaptureScreenshot(HWND hWnd, HWND target)
{
	RECT rect;

	if (target == NULL)
	{
		auto dpi = GetDpiForWindow(GetDesktopWindow());
		rect.left = GetSystemMetricsForDpi(SM_XVIRTUALSCREEN, dpi);
		rect.top = GetSystemMetricsForDpi(SM_YVIRTUALSCREEN, dpi);
		rect.right = GetSystemMetricsForDpi(SM_CXVIRTUALSCREEN, dpi);
		rect.bottom = GetSystemMetricsForDpi(SM_CYVIRTUALSCREEN, dpi);
	}
	else
	{
		GetClientRect(target, &rect);
	}


	auto ddc = GetDC(target);
	if (!ddc)
	{
		MessageBox(hWnd, L"error", L"error", MB_OK);
		return;
	}
	auto tdc = CreateCompatibleDC(ddc);
	if (!tdc)
	{
		MessageBox(hWnd, L"error", L"error", MB_OK);
		ReleaseDC(NULL, ddc);
		return;
	}
	HBITMAP hbmp = CreateCompatibleBitmap(ddc, rect.right, rect.bottom);
	if (!hbmp)
	{
		MessageBox(hWnd, L"error", L"error", MB_OK);
		DeleteObject(tdc);
		ReleaseDC(NULL, ddc);
		return;
	}
	SelectObject(tdc, hbmp);


	if (!BitBlt(tdc, 0, 0, rect.right, rect.bottom, ddc, rect.left, rect.top, SRCCOPY))
	{
		MessageBox(hWnd, L"error", L"error", MB_OK);
		DeleteObject(hbmp);
		DeleteObject(tdc);
		ReleaseDC(NULL, ddc);
		return;
	}

	GetObject(hbmp, sizeof(BITMAP), &bmp);
	BITMAPINFOHEADER   bi;

	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bmp.bmWidth;
	bi.biHeight = bmp.bmHeight;
	bi.biPlanes = bmp.bmPlanes;
	bi.biBitCount = bmp.bmBitsPixel;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;


	DWORD dwBmpSize = ((bmp.bmWidth * bmp.bmBitsPixel + 31) / 32) * 4 * bmp.bmHeight;
	bi.biSizeImage = dwBmpSize;

	bitmapData.resize(dwBmpSize);

	auto ret = GetDIBits(tdc, hbmp, 0, bmp.bmHeight, &bitmapData[0], (BITMAPINFO*)&bi, DIB_RGB_COLORS);

	if (!ret)
	{
		bitmapIsValid = false;
		bitmapData.clear();
	}
	else
		bitmapIsValid = true;


	DeleteObject(hbmp);
	DeleteObject(tdc);
	ReleaseDC(NULL, ddc);

	scrollX = 0;
	scrollY = 0;

	ValidateWindow(hWnd);


	InvalidateRect(hWnd, NULL, FALSE);
}

void ScreenshotUploader::SavePNG(std::string filename)
{
	std::vector<unsigned char> png;
	auto error = GetPNG(png);
	if (error) {
		std::cout << lodepng_error_text(error) << "\n";
		return;
	}
	error = lodepng::save_file(png, filename);
	if (error)
	{
		std::cout << lodepng_error_text(error) << "\n";
		return;
	}
}

void ScreenshotUploader::Draw(HWND hWnd, PAINTSTRUCT ps)
{
	Gdiplus::Color clearColor(255, 180, 180, 180);
	if (!bitmapIsValid)
	{
		if (ps.fErase)
		{
			Gdiplus::Graphics bk(ps.hdc);
			bk.Clear(clearColor);
		}
		return;
	}


	HDC buffer = CreateCompatibleDC(ps.hdc);
	RECT clientRect;
	//GetClientRect(hWnd, &clientRect);
	clientRect = ps.rcPaint;
	HBITMAP hbmp = CreateCompatibleBitmap(ps.hdc, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
	SelectObject(buffer, hbmp);
	Gdiplus::Graphics graphics(buffer);

	// TODO: Add any drawing code that uses hdc here...
	BITMAPINFOHEADER   bi;

	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bmp.bmWidth;
	bi.biHeight = bmp.bmHeight;
	bi.biPlanes = bmp.bmPlanes;
	bi.biBitCount = bmp.bmBitsPixel;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;


	DWORD dwBmpSize = ((bmp.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmp.bmHeight;
	bi.biSizeImage = dwBmpSize;

	BITMAPINFO bminfo;
	bminfo.bmiHeader = bi;


	Gdiplus::Bitmap bm(&bminfo, &bitmapData[0]);

	auto width = min(bmp.bmWidth - scrollX, clientRect.right - clientRect.left);
	auto height = min(bmp.bmHeight - scrollY, clientRect.bottom - clientRect.top);


	graphics.Clear(clearColor);


	int imgposX = 0, imgposY = 0;
	int drawposX = doffsetX - clientRect.left;
	int drawposY = doffsetY - clientRect.top;
	if (drawposX < 0)
	{
		imgposX = drawposX;
		drawposX = 0;
	}
	if (drawposY < 0)
	{
		imgposY = drawposY;
		drawposY = 0;
	}

	graphics.DrawImage(&bm, drawposX, drawposY, scrollX - imgposX, scrollY - imgposY, width, height, Gdiplus::Unit::UnitPixel);


	if (selection.isSelected)
	{
		Gdiplus::Pen pen(Gdiplus::Color(255, 255, 0, 0), penwidth);
		int lx = min(selection.bx, selection.ex);
		int rx = max(selection.bx, selection.ex);
		int ty = min(selection.by, selection.ey);
		int by = max(selection.by, selection.ey);

		graphics.DrawRectangle(&pen, lx - scrollX - clientRect.left + doffsetX, ty - scrollY - clientRect.top + doffsetY, rx - lx, by - ty);
#if _DEBUG
		std::cout << lx << ", " << ty << ", " << rx << ", " << by << "\n";
#endif
	}
	if (!BitBlt(ps.hdc, clientRect.left, clientRect.top, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, graphics.GetHDC(), 0, 0, SRCCOPY))
	{
#if _DEBUG
		std::cout << GetLastError() << "\n";
#endif
	}

	DeleteObject(hbmp);
	DeleteDC(buffer);
}

