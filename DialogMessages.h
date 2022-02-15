#pragma once
#include <Windows.h>
#include <string>

#define WM_CONNECTION_STATUS (WM_USER + 1)


enum class ConnMessage : WPARAM
{
	CONNECTING,
	HANDSHAKING,
	AUTH,
	SENDBEGIN,
	SENDPROGRESS,
	SUCCESS,
	FAIL
};

static std::wstring conntostring(ConnMessage msg)
{
	switch (msg)
	{
	case ConnMessage::CONNECTING:
		return L"CONNECTING";
	case ConnMessage::HANDSHAKING:
		return L"HANDSHAKING";
	case ConnMessage::AUTH:
		return L"AUTH";
	case ConnMessage::SENDBEGIN:
		return L"SENDBEGIN";
	case ConnMessage::SENDPROGRESS:
		return L"SENDPROGRESS";
	case ConnMessage::SUCCESS:
		return L"SUCCESS";
	case ConnMessage::FAIL:
		return L"FAILED";
	default:
		break;
	}
	return L"";
}

