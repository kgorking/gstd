module;
#include <windows.h>
export module gs:file_impl;
import std;
import :Reader;
import :LineReader;
import :Writer;
import :string;

export namespace os {
	constexpr int O_RD = 0x0001; // read
	constexpr int O_WR = 0x0002; // write
	constexpr int O_RDWR = O_RD | O_WR; // read/write
	constexpr int O_ATE = 0x0004; // open at end
	constexpr int O_APP = 0x0008; // always write at end
	constexpr int O_TRUNC = 0x0010; // truncate (open and discard contents)
	constexpr int O_BIN = 0x0020; // binary mode (don't translate newlines)
	constexpr int O_CREATE = O_WR | O_TRUNC;

	class file final {
		HANDLE handle;
		bool eof_flag;

	public:
		file(string path) {
			handle = INVALID_HANDLE_VALUE;
			eof_flag = false;
			open(path, O_RD | O_BIN);
		}

		file(string path, int flags) {
			handle = INVALID_HANDLE_VALUE;
			eof_flag = false;
			open(path, flags);
		}

		file(const file&) = delete;
		file& operator=(const file&) = delete;

		file(file&& other) noexcept : handle(other.handle), eof_flag(other.eof_flag) {
			other.handle = INVALID_HANDLE_VALUE;
			other.eof_flag = false;
		}

		file& operator=(file&& other) noexcept {
			if (this != &other) {
				close();
				handle = other.handle;
				eof_flag = other.eof_flag;
				other.handle = INVALID_HANDLE_VALUE;
				other.eof_flag = false;
			}
			return *this;
		}

		~file() {
			close();
		}

		bool open(string path, int flags) {
			close();

			DWORD desired_access = 0;
			DWORD creation_disposition = OPEN_EXISTING;

			if (flags & O_RD) {
				desired_access |= GENERIC_READ;
			}
			if (flags & O_WR) {
				desired_access |= GENERIC_WRITE;
			}

			if (flags & O_CREATE) {
				creation_disposition = CREATE_ALWAYS;
			} else if (flags & O_TRUNC) {
				creation_disposition = TRUNCATE_EXISTING;
			}

			handle = CreateFileA(
				path.c_str(),
				desired_access,
				FILE_SHARE_READ,
				nullptr,
				creation_disposition,
				FILE_ATTRIBUTE_NORMAL,
				nullptr
			);

			if (handle == INVALID_HANDLE_VALUE) {
				return false;
			}

			// If O_ATE flag is set, seek to end
			if (flags & O_ATE) {
				SetFilePointer(handle, 0, nullptr, FILE_END);
			}

			eof_flag = false;
			return true;
		}

		void close() {
			if (handle != INVALID_HANDLE_VALUE) {
				CloseHandle(handle);
				handle = INVALID_HANDLE_VALUE;
				eof_flag = false;
			}
		}

		operator bool() const {
			return handle != INVALID_HANDLE_VALUE;
		}

		bool end_of_file() const {
			return eof_flag;
		}

		std::size_t size() {
			if (handle == INVALID_HANDLE_VALUE)
				return 0;

			LARGE_INTEGER file_size;
			if (!GetFileSizeEx(handle, &file_size)) {
				return 0;
			}
			return static_cast<std::size_t>(file_size.QuadPart);
		}

		std::expected<std::int64_t, std::error_code> read(std::span<char> buf) {
			if (handle == INVALID_HANDLE_VALUE) {
				return std::unexpected(std::make_error_code(std::errc::bad_file_descriptor));
			}

			DWORD bytes_read = 0;
			if (!ReadFile(handle, buf.data(), static_cast<DWORD>(buf.size()), &bytes_read, nullptr)) {
				return std::unexpected(std::make_error_code(std::errc::io_error));
			}

			if (bytes_read == 0) {
				eof_flag = true;
			}

			return static_cast<std::int64_t>(bytes_read);
		}

		std::expected<string, std::error_code> read_line() {
			if (handle == INVALID_HANDLE_VALUE) {
				return std::unexpected(std::make_error_code(std::errc::bad_file_descriptor));
			}

			std::string result;
			char buffer[256];

			while (true) {
				DWORD bytes_read = 0;
				if (!ReadFile(handle, buffer, sizeof(buffer), &bytes_read, nullptr)) {
					if (result.empty()) {
						return std::unexpected(std::make_error_code(std::errc::io_error));
					}
					break;
				}

				if (bytes_read == 0) {
					eof_flag = true;
					break;
				}

				for (DWORD i = 0; i < bytes_read; ++i) {
					if (buffer[i] == '\n') {
						// Found newline - don't include it, move file pointer back to after the newline
						LONG move_back = static_cast<LONG>(bytes_read - i - 1);
						if (move_back > 0) {
							SetFilePointer(handle, -move_back, nullptr, FILE_CURRENT);
						}
						return {result.c_str()};
					}
					result += buffer[i];
				}
			}

			if (!result.empty()) {
				return {result.c_str()};
			}

			return std::unexpected(std::make_error_code(std::errc::io_error));
		}

		std::expected<std::int64_t, std::error_code> write(std::span<const char> buf) {
			if (handle == INVALID_HANDLE_VALUE) {
				return std::unexpected(std::make_error_code(std::errc::bad_file_descriptor));
			}

			DWORD bytes_written = 0;
			if (!WriteFile(handle, buf.data(), static_cast<DWORD>(buf.size()), &bytes_written, nullptr)) {
				return std::unexpected(std::make_error_code(std::errc::io_error));
			}

			return static_cast<std::int64_t>(bytes_written);
		}
	};

	static_assert(Reader<file>);
	static_assert(LineReader<file>);
	static_assert(Writer<file>);

	file open(string path) {
		return { path };
	}

	file open(string path, int flags) {
		return { path, flags };
	}
};
