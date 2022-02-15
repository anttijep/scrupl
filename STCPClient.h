#pragma once
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <windows.h>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>



enum class ClientState
{
	WAITING,
	SENDING
};

class STCPClient : public std::enable_shared_from_this<STCPClient>
{
	std::mutex mutex;
	std::thread thread;
	boost::asio::ssl::stream<boost::asio::ip::tcp::socket>* sock = nullptr;

	ClientState state = ClientState::WAITING;
	HWND dlg = NULL;
	std::atomic<bool> quit = false;
	void TCPHandleConnection(HWND dialog, std::vector<unsigned char> data, std::string filename,
		std::string addr, std::string port, std::string password);

	void CleanUp();

public:
	STCPClient();
	~STCPClient();
	bool BeginSend(HWND dialog, std::vector<unsigned char> data, std::string filename,
	std::string addr, std::string port, std::string password);
	bool ForceQuit();

};







