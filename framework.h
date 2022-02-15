// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once
#if __clang__
#define UNICODE 1
#endif

#if _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include "targetver.h"
#ifndef WINVER
#define WINVER 0x0A00
#endif
#define _WIN32_WINNT 0x0A00

//#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <afxwin.h>

#ifdef _DEBUG
    #define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
    // Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
    // allocations to be of _CLIENT_BLOCK type
#else
    #define DBG_NEW new
#endif


#include <WinSock2.h>
#include <boost/asio.hpp>
#include <windows.h>
#include <gdiplus.h>
#pragma comment (lib, "gdiplus.lib")
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
