// ScreenshotUploader.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "resource.h"
#include <boost/locale.hpp>
#include <boost/program_options.hpp>
#include "ScreenshotUploader.h"
#include "STCPClient.h"
#include <vector>
#include <iostream>
#include "DialogMessages.h"
#include <memory>
#include <fstream>


#include "DialogMessages.h"



#define MAX_LOADSTRING 100
using namespace Gdiplus;

namespace {
	// Global Variables:
	HINSTANCE hInst;                                // current instance
	WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
	WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
	std::unique_ptr<ScreenshotUploader> scruplInstance = nullptr;
	std::shared_ptr<STCPClient> stcpClient = nullptr;
	boost::program_options::variables_map program_options;
	std::wstring GetWstringFromControl(HWND handle, int id)
	{
		HWND ctrl = GetDlgItem(handle, id);
		std::wstring textout;
		int len = GetWindowTextLength(ctrl) + 1;
		textout.resize(len);
		GetWindowText(ctrl, &textout[0], len);
		textout.pop_back();
		return textout;
	}

	std::string GetTextFromControl(HWND handle, int id, UINT codePage = CP_UTF8)
	{
		HWND fnameh = GetDlgItem(handle, id);

		std::string textout;
		int buffersize = GetWindowTextLength(fnameh) + 1;
		if (buffersize > 0)
		{
			std::wstring buffer;
			buffer.resize(buffersize);

			GetWindowText(fnameh, &buffer[0], buffersize);

			buffersize = WideCharToMultiByte(codePage, NULL, buffer.c_str(), -1, NULL, 0, NULL, NULL) + 1;

			textout.resize(buffersize);

			buffersize = WideCharToMultiByte(codePage, NULL, buffer.c_str(), -1, &textout[0], buffersize, NULL, NULL);
			textout.resize(buffersize - 1);
		}
		return textout;
	}


	BOOL SetControlText(HWND handle, int id, const std::wstring& txt)
	{
		return SetWindowText(GetDlgItem(handle, id), txt.c_str());
	}

	void AddProgramOption(std::string option, std::wstring value, bool xdefaulted = false)
	{
		namespace po = boost::program_options;
		if (program_options.count(option))
		{
			program_options.erase(option);
		}
		program_options.insert(std::pair<const std::string, po::variable_value>(option, po::variable_value::variable_value(boost::any(std::move(value)), xdefaulted)));
	}

}



// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    SendFileDialog(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
#if _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

#if _DEBUG
	FILE* fp;

	AllocConsole();
	freopen_s(&fp, "CONIN$", "r", stdin);
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONOUT$", "w", stderr);
#endif
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;

	namespace po = boost::program_options;
	po::options_description config("Options");
	config.add_options()("address", po::wvalue<std::wstring>())
		("port", po::wvalue<std::wstring>());
	{
		std::wifstream ifs("config.ini");

		if (ifs.is_open())
		{
			po::store(boost::program_options::parse_config_file<wchar_t>(ifs, config), program_options);
			po::notify(program_options);
			ifs.close();
		}
		if (!program_options.count("port"))
		{
			AddProgramOption("port", L"42715");
		}

	}


	scruplInstance = std::make_unique<ScreenshotUploader>();
	stcpClient = std::make_shared<STCPClient>();
	// TODO: Place code here.
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_SCREENSHOTUPLOADER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SCREENSHOTUPLOADER));

	MSG msg;

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	GdiplusShutdown(gdiplusToken);
	{
		std::wofstream stream("config.ini");
		if (stream.is_open())
		{
			if (program_options.count("port"))
			{
					stream << L"port=" << program_options["port"].as<std::wstring>() << "\n";
			}
			if (program_options.count("address"))
			{
				stream << L"address=" << program_options["address"].as<std::wstring>() << "\n";
			}

			stream.close();
		}

	}
#if _DEBUG
	_CrtDumpMemoryLeaks();
#endif
	return (int)msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SCREENSHOTUPLOADER));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = nullptr;
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_SCREENSHOTUPLOADER);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	AfxInitRichEdit2();
	InitCommonControls();
	INITCOMMONCONTROLSEX initctrlex;
	initctrlex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	initctrlex.dwICC = ICC_PROGRESS_CLASS;
	InitCommonControlsEx(&initctrlex);
	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	if (!RegisterHotKey(hWnd, 1, NULL, VK_SNAPSHOT))
	{
		std::cout << "fail\n";
	}
	if (!RegisterHotKey(hWnd, 2, MOD_CONTROL, VK_SNAPSHOT))
	{
		std::cout << "fail\n";
	}

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	switch (message)
	{
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		case ID_FILE_SENDFILE:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_SENDFILE), hWnd, SendFileDialog);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_SIZE:
	{
		if (!scruplInstance->HandleResize(hWnd, wParam, lParam))
			return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}
	case WM_HOTKEY:

		switch (HIWORD(lParam))
		{
		case (VK_SNAPSHOT):
		{
			if (LOWORD(lParam) == MOD_CONTROL)
			{
				scruplInstance->CaptureScreenshot(hWnd, GetForegroundWindow());
			}
			else
			{
				scruplInstance->CaptureScreenshot(hWnd);
			}
		}
		}

		break;
	case WM_KEYDOWN:
	{
#if _DEBUG
		WCHAR keyName[256];
		int retval = GetKeyNameText(lParam, keyName, 256);
		if (retval)
		{
			std::wcout << keyName << "\n";
		}
#endif
		switch (wParam)
		{
		case (VK_F9):
			scruplInstance->SavePNG("testi.png");
			break;
		case (VK_F11):
		{
			scruplInstance->CaptureScreenshot(hWnd);
		}
		break;
		case (VK_F7):
		{
			scruplInstance->CaptureScreenshot(hWnd, hWnd);
		}
		break;
		case (VK_F6):
		{
			DialogBox(hInst, MAKEINTRESOURCE(IDD_SENDFILE), hWnd, SendFileDialog);
		}
		break;
		default:
			if (!scruplInstance->KeyDown(hWnd, wParam, lParam))
				return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_MOUSEWHEEL:
		if (LOWORD(wParam) & MK_SHIFT) {
			if (!scruplInstance->HandleHScroll(hWnd, wParam, lParam, HIWORD(wParam)))
				return DefWindowProc(hWnd, message, wParam, lParam);
		}
		else {
			if (!scruplInstance->HandleVScroll(hWnd, wParam, lParam, HIWORD(wParam)))
				return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_HSCROLL:
		if (!scruplInstance->HandleHScroll(hWnd, wParam, lParam))
			return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	case WM_VSCROLL:
		if (!scruplInstance->HandleVScroll(hWnd, wParam, lParam))
			return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MOUSEMOVE:
		if (!scruplInstance->HandleMouse(message, hWnd, wParam, lParam))
			return DefWindowProcW(hWnd, message, wParam, lParam);
		break;
		//	case WM_ERASEBKGND:
			//	return TRUE;
	case WM_PAINT:
	{

		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		scruplInstance->Draw(hWnd, ps);
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}



INT_PTR CALLBACK    SendFileDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		int focus = IDC_ADDRESS;
		if (program_options.count("address"))
		{
			SetControlText(hDlg, IDC_ADDRESS, program_options["address"].as<std::wstring>());
			focus = IDC_PORT;
		}
		else
		{
			focus = IDC_ADDRESS;
		}
		if (program_options.count("port"))
		{
			SetControlText(hDlg, IDC_PORT, program_options["port"].as<std::wstring>());
			if (focus == IDC_PORT)
			{
				focus = IDC_PASSWORD;
			}
		}
		SetFocus(GetDlgItem(hDlg, focus));

	}
	return (INT_PTR)FALSE;
	case WM_CONNECTION_STATUS:
		switch ((ConnMessage)LOWORD(wParam))
		{
		case (ConnMessage::FAIL):
			EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
			EnableWindow(GetDlgItem(hDlg, IDC_ADDRESS), TRUE);
			EnableWindow(GetDlgItem(hDlg, IDC_PORT), TRUE);
			EnableWindow(GetDlgItem(hDlg, IDC_PASSWORD), TRUE);
			EnableWindow(GetDlgItem(hDlg, IDC_FILENAME), TRUE);
			SetControlText(hDlg, IDC_PROGRESSTEXT, GetWstringFromControl(hDlg, IDC_PROGRESSTEXT) + L" FAILED");
			break;
		case (ConnMessage::SUCCESS):
			ShowWindow(GetDlgItem(hDlg, IDC_PROGRESSTEXT), SW_SHOW);
			if (lParam != NULL)
			{
				SetControlText(hDlg, IDC_PROGRESSTEXT, (wchar_t*)lParam);
				SetControlText(hDlg, IDCANCEL, L"CLOSE");
				EnableWindow(GetDlgItem(hDlg, IDC_PROGRESSTEXT), TRUE);
				ShowWindow(GetDlgItem(hDlg, IDC_TOCLIPBOARD), TRUE);

				AddProgramOption("port", GetWstringFromControl(hDlg, IDC_PORT));
				AddProgramOption("address", GetWstringFromControl(hDlg, IDC_ADDRESS));
			}
			else
			{
				SetControlText(hDlg, IDC_PROGRESSTEXT, L"Server returned empty url");
			}
			break;
		case (ConnMessage::SENDBEGIN):
		{
			HWND handle = GetDlgItem(hDlg, IDC_SENDPROGRESS);
			ShowWindow(handle, SW_SHOW);
			std::wstring msgtotal = L"Sending...";
			SetControlText(hDlg, IDC_PROGRESSTEXT, msgtotal.c_str());
			SendMessage(handle, PBM_SETRANGE, 0, MAKELPARAM(0, lParam));
			SendMessage(GetDlgItem(hDlg, IDC_SENDPROGRESS), PBM_SETPOS, 0, 0);
		}
		break;
		case ConnMessage::SENDPROGRESS:
			SendMessage(GetDlgItem(hDlg, IDC_SENDPROGRESS), PBM_SETPOS, lParam, 0);
			UpdateWindow(hDlg);
			break;
		default:
			SetControlText(hDlg, IDC_PROGRESSTEXT, conntostring((ConnMessage)wParam));
			break;
		}
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			std::string filename = GetTextFromControl(hDlg, IDC_FILENAME);
			std::string password = GetTextFromControl(hDlg, IDC_PASSWORD);
			std::string address = GetTextFromControl(hDlg, IDC_ADDRESS);
			std::string port = GetTextFromControl(hDlg, IDC_PORT);

			std::vector<unsigned char> data;
			std::wstring msg;
			if (scruplInstance->GetPNG(data)) {
				msg = L"Failed to convert screenshot to png";
				SetControlText(hDlg, IDC_PROGRESSTEXT, msg.c_str());
				break;
			}
			msg = L"Sending file";
			SetControlText(hDlg, IDC_PROGRESSTEXT, msg.c_str());
			if (stcpClient->BeginSend(hDlg, std::move(data), std::move(filename), std::move(address), std::move(port), std::move(password)))
			{
				EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_ADDRESS), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_PORT), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_PASSWORD), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_FILENAME), FALSE);
			}
			else {
				msg = L"Send failed";
				SetControlText(hDlg, IDC_PROGRESSTEXT, msg.c_str());
			}
		}
		else if (LOWORD(wParam) == IDCANCEL)
		{
			stcpClient->ForceQuit();
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDC_TOCLIPBOARD)
		{
			HWND hctrl = GetDlgItem(hDlg, IDC_PROGRESSTEXT);
			int buffersize = GetWindowTextLength(hctrl) + 1;
			if (buffersize == 1)
			{
				break;
			}
			if (!OpenClipboard(hDlg))
			{
				break;
			}


			HGLOBAL buffer = GlobalAlloc(GMEM_MOVEABLE, buffersize * sizeof(TCHAR));

			if (!buffer)
			{
				CloseClipboard();
				break;
			}
			LPTSTR str = (LPTSTR)GlobalLock(buffer);
			if (!str)
			{
				CloseClipboard();
				break;
			}
			GetWindowText(hctrl, str, buffersize);
			GlobalUnlock(str);
			EmptyClipboard();
			SetClipboardData(CF_UNICODETEXT, str);
			CloseClipboard();
			return (INT_PTR)TRUE;
		}
		break;
	default:
		return (INT_PTR)FALSE;
	}
	return (INT_PTR)TRUE;
}


