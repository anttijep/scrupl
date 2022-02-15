#pragma once

#include "framework.h"
#include "resource.h"
#include <string>
#include <vector>




struct Selection
{
	int bx = 0, by = 0, ex = 0, ey = 0;
	bool isSelected = false;
	bool isSelecting = false;
};

class ScreenshotUploader
{
	BITMAP bmp;
	std::vector<char> bitmapData;
	bool bitmapIsValid = false;
	bool restoreWindow = false;
	Selection selection;
	Gdiplus::REAL penwidth = 2;

	int scrollX = 0,
		scrollY = 0;
	int doffsetX = 0,
		doffsetY = 0;


public:
	ScreenshotUploader();
	~ScreenshotUploader();
	bool KeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam);

	bool HandleResize(HWND hWnd, WPARAM wParam, LPARAM lParam);
	bool HandleVScroll(HWND hWnd, WPARAM wParam, LPARAM lParam, short fixed = 0x7fff);
	bool HandleHScroll(HWND hWnd, WPARAM wParam, LPARAM lParam, short fixed = 0x7fff);
	bool HandleMouse(UINT message, HWND hWnd, WPARAM wParam, LPARAM lParam);


	void ValidateWindow(HWND hWnd);

	unsigned int GetPNG(std::vector<unsigned char>& pngOut);
	

	void CaptureScreenshot(HWND hWnd, HWND target = NULL);

	void SavePNG(std::string filename);
	void Draw(HWND hWnd, PAINTSTRUCT ps);
};



