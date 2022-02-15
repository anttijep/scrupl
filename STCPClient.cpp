

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include "STCPClient.h"
#include <assert.h>
#include <wincrypt.h>

#pragma comment(lib, "crypt32.lib")


#include "DialogMessages.h"

#if _DEBUG
#include <iostream>
#define DEBUGPRINT(x,y) std::cout << x << y <<  "\n"
//#define DEBUGDELAY() std::this_thread::sleep_for(std::chrono::seconds(1))
#define DEBUGDELAY()
#else
#define DEBUGDELAY()
#define DEBUGPRINT(x,y)
#define DEBUGPRINT(x)
#endif

void GetWindowsCerts(boost::asio::ssl::context& ctx)
{
	HCERTSTORE hStore = CertOpenSystemStore(NULL, L"ROOT");
	if (!hStore)
		return;

	X509_STORE* store = X509_STORE_new();
	PCCERT_CONTEXT pContext = NULL;
	while ((pContext = CertEnumCertificatesInStore(hStore, pContext)) != NULL)
	{
		X509* x509 = d2i_X509(NULL, (const unsigned char**)&pContext->pbCertEncoded, pContext->cbCertEncoded);
		if (x509)
		{
			X509_STORE_add_cert(store, x509);
			X509_free(x509);
		}
	}
	CertFreeCertificateContext(pContext);
	CertCloseStore(hStore, NULL);
	SSL_CTX_set_cert_store(ctx.native_handle(), store);
}

void ReportUpdate(HWND dlg, ConnMessage status, const std::string& msg)
{
	if (msg.size() > 0)
	{
		std::wstring outstr;
		int buffsize = MultiByteToWideChar(CP_UTF8, NULL, msg.c_str(), -1, NULL, NULL);
		outstr.resize(buffsize);

		buffsize = MultiByteToWideChar(CP_UTF8, NULL, msg.c_str(), -1, &outstr[0], outstr.size());

		outstr.resize(buffsize - 1);

		SendMessage(dlg, WM_CONNECTION_STATUS, (WPARAM)status, (LPARAM)outstr.c_str());
	}
	else
	{
		SendMessage(dlg, WM_CONNECTION_STATUS, (WPARAM)status, NULL);
	}
}

void STCPClient::TCPHandleConnection(HWND dialog, std::vector<unsigned char> data, std::string filename,
	std::string addr, std::string port, std::string password)
{
	using boost::asio::ip::tcp;
	namespace ssl = boost::asio::ssl;
	std::unique_lock<std::mutex> lock(this->mutex);
	const static uint64_t DATA_MAX_SIZE = UINT32_MAX;
	if (password.size() > 255 || filename.size() > 255 || data.size() > DATA_MAX_SIZE)
	{
		lock.unlock();
		std::string err = "Password, filename or data too long";
		SendMessage(dialog, WM_CONNECTION_STATUS, (WPARAM)ConnMessage::FAIL, (LPARAM)err.c_str());
		lock.lock();
		this->CleanUp();
		lock.unlock();
		return;
	}
	try
	{
		boost::asio::io_context io_context;
		ssl::context ctx(ssl::context::tls_client);

		GetWindowsCerts(ctx);
		ssl::stream<tcp::socket> sock(io_context, ctx);
		int conn = 1;
		tcp::resolver resolver(io_context);
		tcp::resolver::query query(addr, port);
		this->sock = &sock;
		DEBUGDELAY();
		lock.unlock();

		DEBUGDELAY();

		SendMessage(dialog, WM_CONNECTION_STATUS, (WPARAM)ConnMessage::CONNECTING, NULL);
		DEBUGDELAY();

		boost::asio::connect(sock.lowest_layer(), resolver.resolve(query));
		sock.set_verify_mode(ssl::verify_peer);
		sock.set_verify_callback(ssl::rfc2818_verification(addr));

		ReportUpdate(dialog, ConnMessage::HANDSHAKING, "");
		DEBUGDELAY();

		sock.handshake(ssl::stream_base::handshake_type::client);

		std::vector<unsigned char> sendbuffer;
		sendbuffer.push_back(password.size() & 0xffu);
		sendbuffer.insert(sendbuffer.end(), password.begin(), password.end());

		SendMessage(dialog, WM_CONNECTION_STATUS, (WPARAM)ConnMessage::AUTH, NULL);
		DEBUGDELAY();

		auto ret = boost::asio::write(sock, boost::asio::buffer(sendbuffer));

		std::vector<unsigned char> out;
		std::array<unsigned char, 512> buffer;
		do
		{
			ret = sock.read_some(boost::asio::buffer(buffer));
			if (ret == 0)
				break;
			out.insert(out.end(), buffer.begin(), buffer.begin() + ret);
		} while (out.size() < 2);

		if (out.size() >= 2 && out[0] == 'O' && out[1] == 'K')
		{
			DEBUGDELAY();
			sendbuffer.clear();
			sendbuffer.push_back(filename.size() & 0xffu);
			if (filename.size() > 0)
			{
				sendbuffer.insert(sendbuffer.end(), filename.begin(), filename.end());
			}
			auto datasize = htonl(data.size());
			size_t sbsize = sendbuffer.size();
			sendbuffer.resize(sendbuffer.size() + sizeof(u_long));
			memcpy(&sendbuffer[sbsize], &datasize, sizeof(u_long));
			size_t totalsize = sendbuffer.size() + data.size();
			size_t totalprogress = 0;
			size_t crrprogress = 0;

			SendMessage(dialog, WM_CONNECTION_STATUS, (WPARAM)ConnMessage::SENDBEGIN, 100);

			do
			{
				size_t wrote = sock.write_some(boost::asio::buffer(&sendbuffer[crrprogress], sendbuffer.size() - crrprogress));
				crrprogress += wrote;
				totalprogress += wrote;
				SendMessage(dialog, WM_CONNECTION_STATUS, (WPARAM)ConnMessage::SENDPROGRESS, (totalprogress * 100) / totalsize);
				DEBUGDELAY();
			} while (crrprogress < sendbuffer.size());

			crrprogress = 0;
			do
			{
				size_t wrote = sock.write_some(boost::asio::buffer(&data[crrprogress], data.size() - crrprogress));
				crrprogress += wrote;
				totalprogress += wrote;
				SendMessage(dialog, WM_CONNECTION_STATUS, (WPARAM)ConnMessage::SENDPROGRESS, (totalprogress * 100) / totalsize);
				DEBUGDELAY();
			} while (crrprogress < data.size());
			//	boost::asio::write(sock, boost::asio::buffer(sendbuffer));
			//	boost::asio::write(sock, boost::asio::buffer(data));

			out.clear();
			do
			{
				ret = sock.read_some(boost::asio::buffer(buffer));
				if (ret == 0)
					break;
				out.insert(out.end(), buffer.begin(), buffer.begin() + ret);
			} while (out.size() < 2);

			if (out.size() >= 2)
			{
				u_short ln;
				memcpy(&ln, &buffer[0], 2);
				auto length = ntohs(ln);
				while (out.size() < length)
				{
					ret = sock.read_some(boost::asio::buffer(buffer));
					if (ret == 0)
						break;
					out.insert(out.end(), buffer.begin(), buffer.begin() + ret);
				}
				for (size_t i = 2; i < out.size(); ++i)
				{
					out[i - 2] = out[i];
				}
				out.resize(out.size() - 2);
			}
			std::string msg(out.begin(), out.end());
			ReportUpdate(dialog, ConnMessage::SUCCESS, msg);
			DEBUGDELAY();
		}
		else
		{
			std::string msg(out.begin(), out.end());
			ReportUpdate(dialog, ConnMessage::FAIL, msg);
			DEBUGDELAY();
		}
	}
	catch (std::exception& e)
	{
		SendMessage(dialog, WM_CONNECTION_STATUS, (WPARAM)ConnMessage::FAIL, (LPARAM)e.what());
		DEBUGDELAY();
		DEBUGPRINT("Exception: ", e.what());
	}



	if (!lock.owns_lock())
		lock.lock();
	this->CleanUp();

	lock.unlock();
}

void STCPClient::CleanUp()
{
	quit = false;
	state = ClientState::WAITING;
	sock = nullptr;
}

STCPClient::STCPClient()
{
}

STCPClient::~STCPClient()
{
}

bool STCPClient::BeginSend(HWND dialog, std::vector<unsigned char> data, std::string filename,
	std::string addr, std::string port, std::string password)
{
	if (password.empty() || data.empty() || addr.empty() || port.empty())
		return false;
	std::unique_lock<std::mutex> lock(mutex, std::try_to_lock);
	if (!lock.owns_lock())
		return false;
	if (state != ClientState::WAITING)
		return false;
	quit = false;

	state = ClientState::SENDING;

	thread = std::thread(&STCPClient::TCPHandleConnection, shared_from_this(), dialog, std::move(data), std::move(filename), std::move(addr), std::move(port), std::move(password));
	thread.detach();
	return true;
}

bool STCPClient::ForceQuit()
{
	std::lock_guard<std::mutex> lock(mutex);
	if (quit)
	{
		return false;
	}
	quit = true;
	if (sock != nullptr && sock->lowest_layer().is_open())
	{
		sock->async_shutdown([](const boost::system::error_code& e) { DEBUGPRINT("Failed to shutdown socket: ", e.message()); });
	}
	return true;
}



