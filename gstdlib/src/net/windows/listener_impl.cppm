module;
#include <cstring>
#include <stdexcept>
#include <system_error>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <afunix.h>

export module gs:listener_impl;
import :socket;
import :string;
import :types;
import std;

namespace {
	using socket_handle = SOCKET;
	using port_type = u_short;
	using socklen_type = int;
	constexpr socket_handle invalid_socket = INVALID_SOCKET;
	constexpr int socket_error = SOCKET_ERROR;

	inline int close_socket(socket_handle const fd) noexcept {
		return ::closesocket(fd);
	}

	inline int socket_last_error() noexcept {
		return ::WSAGetLastError();
	}

	inline std::error_category const& socket_error_category() noexcept {
		return std::system_category();
	}
}

export namespace net {
	class listener {
		constexpr static socket_handle invalid_socket = socket_handle{} - 1;
		socket_handle fd_ = invalid_socket;
	public:
		listener() = default;

		listener(socket s, string network, string address) {
			fd_ = s.fd_;
			s.fd_ = invalid_socket;

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
				addr_in->sin_port = htons(static_cast<port_type>(std::stoi(port_str)));
				if (host_str.empty()) addr_in->sin_addr.s_addr = htonl(INADDR_ANY);
				else if (inet_pton(AF_INET, host_str.c_str(), &addr_in->sin_addr) != 1) {
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
					if (colon == std::string::npos) {
						close();
						throw std::invalid_argument("missing port in address: " + addr_str);
					}
					host_str = addr_str.substr(0, colon);
					port_str = addr_str.substr(colon + 1);
				}
				addr_in6->sin6_port = htons(static_cast<port_type>(std::stoi(port_str)));
				if (host_str.empty()) addr_in6->sin6_addr = in6addr_any;
				else if (inet_pton(AF_INET6, host_str.c_str(), &addr_in6->sin6_addr) != 1) {
					close();
					throw std::invalid_argument("invalid IP address: " + host_str);
				}
			} else if (network == "unix" || network == "unixpacket") {
				auto* addr_un = reinterpret_cast<sockaddr_un*>(&ss);
				addr_un->sun_family = AF_UNIX;
				if (addr_str.size() >= sizeof(addr_un->sun_path)) {
					close();
					throw std::runtime_error("unix socket path too long");
				}
				std::memcpy(addr_un->sun_path, addr_str.c_str(), addr_str.size() + 1);
			} else {
				close();
				throw std::invalid_argument("unsupported network: " + std::string(network.c_str(), network.size_bytes()));
			}

			socklen_type addr_len = static_cast<socklen_type>(sizeof(ss));
			if (bind(fd_, reinterpret_cast<sockaddr*>(&ss), addr_len) == socket_error) {
				int const err = socket_last_error();
				close();
				throw std::system_error(err, socket_error_category(), "bind failed");
			}
			if (listen(fd_, SOMAXCONN) == socket_error) {
				int const err = socket_last_error();
				close();
				throw std::system_error(err, socket_error_category(), "listen failed");
			}
		}

		~listener() { close(); }
		listener(listener&& o) noexcept : fd_(std::exchange(o.fd_, invalid_socket)) {}
		listener& operator=(listener&& o) noexcept {
			if (this != &o) {
				close();
				fd_ = std::exchange(o.fd_, invalid_socket);
			}
			return *this;
		}
		listener(listener const&) = delete;
		listener& operator=(listener const&) = delete;

		socket accept() {
			sockaddr_storage client_addr = {};
			socklen_type client_addr_len = sizeof(client_addr);
			socket_handle const client = ::accept(fd_, reinterpret_cast<sockaddr*>(&client_addr), &client_addr_len);
			return socket(client);
		}

		bool valid() const { return fd_ != invalid_socket; }
		void close() {
			if (fd_ != invalid_socket) {
				close_socket(fd_);
				fd_ = invalid_socket;
			}
		}
	};

	listener listen(string network, string address) {
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
