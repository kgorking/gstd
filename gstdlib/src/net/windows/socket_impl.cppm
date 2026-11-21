module;
#include <cstdlib>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>

export module gs:socket_impl;
import :Reader;
import :Writer;
import :types;
import :concepts;
import std;

namespace {
	using socket_size = int64;
	using socket_handle = SOCKET;

	inline int close_socket(socket_handle const fd) noexcept {
		return ::closesocket(fd);
	}

	inline int socket_last_error() noexcept {
		return ::WSAGetLastError();
	}

	inline std::error_category const& socket_error_category() noexcept {
		return std::system_category();
	}

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
		constexpr static socket_handle invalid_socket = socket_handle{} - 1;
		socket_handle fd_ = invalid_socket;
	public:
		socket() = default;

		static socket create(int family, int type, int protocol) {
			socket_handle const s = ::socket(family, type, protocol);
			if (s == invalid_socket) {
				throw std::system_error(socket_last_error(), socket_error_category(), "socket failed");
			}
			return socket(s);
		}

		~socket() {
			close();
		}

		socket(socket&& o) noexcept : fd_(std::exchange(o.fd_, invalid_socket)) {}

		socket& operator=(socket&& o) noexcept {
			if (this != &o) {
				close();
				fd_ = std::exchange(o.fd_, invalid_socket);
			}
			return *this;
		}

		socket(socket const&) = delete;
		socket& operator=(socket const&) = delete;

		bool valid() const { return fd_ != invalid_socket; }

		void close() {
			if (fd_ != invalid_socket) {
				close_socket(fd_);
				fd_ = invalid_socket;
			}
		}

		int64 write(Span<const char> auto data) const {
			return ::send(fd_, data.data(), static_cast<socket_size>(data.size()), 0);
		}

		int64 read(Span<char> auto& buf) const {
			return ::recv(fd_, buf.data(), static_cast<socket_size>(buf.size()), 0);
		}

	private:
		friend class listener;
		explicit socket(socket_handle const fd) : fd_(fd) {}
	};

	static_assert(Reader<socket>);
	static_assert(Writer<socket>);
}
