module;
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <afunix.h>
#pragma comment(lib, "ws2_32.lib")

export module gs:listener;
import :socket;
import :string;
import :types;
import std;

export namespace net {
	class listener {
		SOCKET fd_ = INVALID_SOCKET;
	public:
		listener() = default;

		listener(socket s, string network, string address) {
			fd_ = s.fd_;
			s.fd_ = INVALID_SOCKET;

			std::string addr_str(address.c_str(), address.size_bytes());
			sockaddr_storage ss = {};

			if (network == "tcp" || network == "tcp4") {
				auto* addr_in = reinterpret_cast<sockaddr_in*>(&ss);
				addr_in->sin_family = AF_INET;

				auto colon = addr_str.find(':');
				if (colon == std::string::npos) {
					close();
					throw std::invalid_argument("missing port in address: " + addr_str);
				}
				std::string host_str = (colon == 0) ? "" : addr_str.substr(0, colon);
				std::string port_str = addr_str.substr(colon + 1);
				addr_in->sin_port = htons(static_cast<u_short>(std::stoi(port_str)));

				if (host_str.empty()) {
					addr_in->sin_addr.s_addr = htonl(INADDR_ANY);
				} else if (inet_pton(AF_INET, host_str.c_str(), &addr_in->sin_addr) != 1) {
					close();
					throw std::invalid_argument("invalid IP address: " + host_str);
				}
			} else if (network == "tcp6") {
				auto* addr_in6 = reinterpret_cast<sockaddr_in6*>(&ss);
				addr_in6->sin6_family = AF_INET6;

				std::string host_str;
				std::string port_str;

				if (addr_str[0] == '[') {
					auto close_bracket = addr_str.find(']');
					if (close_bracket == std::string::npos || close_bracket + 1 >= addr_str.size() || addr_str[close_bracket + 1] != ':') {
						close();
						throw std::invalid_argument("invalid IPv6 address format: " + addr_str);
					}
					host_str = addr_str.substr(1, close_bracket - 1);
					port_str = addr_str.substr(close_bracket + 2);
				} else {
					auto colon = addr_str.rfind(':');
					if (colon == std::string::npos || colon == 0) {
						close();
						throw std::invalid_argument("missing port in address: " + addr_str);
					}
					host_str = addr_str.substr(0, colon);
					port_str = addr_str.substr(colon + 1);
				}

				addr_in6->sin6_port = htons(static_cast<u_short>(std::stoi(port_str)));

				if (host_str.empty()) {
					addr_in6->sin6_addr = in6addr_any;
				} else if (inet_pton(AF_INET6, host_str.c_str(), &addr_in6->sin6_addr) != 1) {
					close();
					throw std::invalid_argument("invalid IPv6 address: " + host_str);
				}
			} else if (network == "unix" || network == "unixpacket") {
				auto* addr_un = reinterpret_cast<sockaddr_un*>(&ss);
				addr_un->sun_family = AF_UNIX;

				if (addr_str.size() >= sizeof(addr_un->sun_path)) {
					close();
					throw std::runtime_error("unix socket path too long");
				}
				memcpy(addr_un->sun_path, addr_str.c_str(), addr_str.size() + 1);
			} else {
				close();
				throw std::invalid_argument("unsupported network: " + std::string(network.c_str(), network.size_bytes()));
			}

			int addr_len = static_cast<int>(sizeof(ss));
			if (bind(fd_, reinterpret_cast<sockaddr*>(&ss), addr_len) == SOCKET_ERROR) {
				DWORD err = WSAGetLastError();
				close();
				throw std::system_error(err, std::system_category(), "bind failed");
			}

			if (listen(fd_, SOMAXCONN) == SOCKET_ERROR) {
				DWORD err = WSAGetLastError();
				close();
				throw std::system_error(err, std::system_category(), "listen failed");
			}
		}

		~listener() {
			close();
		}

		listener(listener&& o) noexcept : fd_(std::exchange(o.fd_, INVALID_SOCKET)) {}

		listener& operator=(listener&& o) noexcept {
			if (this != &o) {
				close();
				fd_ = std::exchange(o.fd_, INVALID_SOCKET);
			}
			return *this;
		}

		listener(listener const&) = delete;
		listener& operator=(listener const&) = delete;

		socket accept() {
			sockaddr_storage client_addr = {};
			int client_addr_len = sizeof(client_addr);
			SOCKET client = ::accept(fd_, reinterpret_cast<sockaddr*>(&client_addr), &client_addr_len);
			return socket(client);
		}

		bool valid() const { return fd_ != INVALID_SOCKET; }

		void close() {
			if (fd_ != INVALID_SOCKET) {
				closesocket(fd_);
				fd_ = INVALID_SOCKET;
			}
		}
	};

	export listener listen(string network, string address) {
		int family;
		int type;
		int protocol;

		if (network == "tcp" || network == "tcp4") {
			family = AF_INET; type = SOCK_STREAM; protocol = IPPROTO_TCP;
		} else if (network == "tcp6") {
			family = AF_INET6; type = SOCK_STREAM; protocol = IPPROTO_TCP;
		} else if (network == "unix") {
			family = AF_UNIX; type = SOCK_STREAM; protocol = 0;
		} else if (network == "unixpacket") {
			family = AF_UNIX; type = SOCK_SEQPACKET; protocol = 0;
		} else {
			throw std::invalid_argument("unsupported network: " + std::string(network.c_str(), network.size_bytes()));
		}

		net::socket s = net::socket::create(family, type, protocol);
		return listener(std::move(s), network, address);
	}
}
