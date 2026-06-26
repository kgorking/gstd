module;
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

export module gs:socket;
import :Reader;
import :Writer;
import :types;
import std;

namespace {
	struct wsa_lifetime {
		wsa_lifetime() {
			WSADATA wsa;
			WSAStartup(MAKEWORD(2, 2), &wsa);
		}
		~wsa_lifetime() { WSACleanup(); }
	};
	wsa_lifetime g_wsa;
}

export namespace net {
	class socket {
		SOCKET fd_ = INVALID_SOCKET;
	public:
		socket() = default;

		static socket create(int family, int type, int protocol) {
			SOCKET s = ::socket(family, type, protocol);
			if (s == INVALID_SOCKET) {
				throw std::system_error(WSAGetLastError(), std::system_category(), "socket failed");
			}
			return socket(s);
		}

		~socket() {
			close();
		}

		socket(socket&& o) noexcept : fd_(std::exchange(o.fd_, INVALID_SOCKET)) {}

		socket& operator=(socket&& o) noexcept {
			if (this != &o) {
				close();
				fd_ = std::exchange(o.fd_, INVALID_SOCKET);
			}
			return *this;
		}

		socket(socket const&) = delete;
		socket& operator=(socket const&) = delete;

		bool valid() const { return fd_ != INVALID_SOCKET; }

		void close() {
			if (fd_ != INVALID_SOCKET) {
				closesocket(fd_);
				fd_ = INVALID_SOCKET;
			}
		}

		int64 write(std::span<const char> data) const {
			return ::send(fd_, data.data(), static_cast<int>(data.size()), 0);
		}

		int64 read(std::span<char> buf) const {
			return ::recv(fd_, buf.data(), static_cast<int>(buf.size()), 0);
		}

	private:
		friend class listener;
		explicit socket(SOCKET fd) : fd_(fd) {}
	};

	static_assert(Reader<socket>);
	static_assert(Writer<socket>);
}
