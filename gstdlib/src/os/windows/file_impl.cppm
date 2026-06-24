module;
#include <windows.h>
export module gs:file_impl;
import std;
import :concepts;
import :Reader;
import :LineReader;
import :Writer;
import :LineWriter;
import :string;
import :task;
import :thread_pool;
import :get_last_error;

export namespace os {
	template<typename T, auto operation>
	class async_io_awaiter {
	private:
		HANDLE file_handle;
		std::span<T> buffer;
		ULONGLONG* file_position;

	public:
		async_io_awaiter(HANDLE h, std::span<T> buf, ULONGLONG* pos)
			: file_handle(h), buffer(buf), file_position(pos) {}

		bool await_ready() const noexcept { return false; }

		void await_suspend(std::coroutine_handle<> cont) {
			thread_pool::instance().enqueue_io(cont);
		}

		std::int64_t await_resume() {
			DWORD bytes_read = 0;
			if (!operation(file_handle, buffer.data(), static_cast<DWORD>(buffer.size()), &bytes_read, nullptr)) {
				throw std::system_error(std::make_error_code(std::errc::io_error));
			}

			*file_position += bytes_read;
			return static_cast<std::int64_t>(bytes_read);
		}
	};

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
		ULONGLONG file_position = 0;

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

		explicit file(HANDLE h) : handle(h), eof_flag(false) {}

		file(const file&) = delete;
		file& operator=(const file&) = delete;

		file(file&& other) noexcept : handle(other.handle), eof_flag(other.eof_flag), file_position(other.file_position) {
			other.handle = INVALID_HANDLE_VALUE;
			other.eof_flag = false;
			other.file_position = 0;
		}

		file& operator=(file&& other) noexcept {
			if (this != &other) {
				close();
				handle = other.handle;
				eof_flag = other.eof_flag;
				file_position = other.file_position;
				other.handle = INVALID_HANDLE_VALUE;
				other.eof_flag = false;
				other.file_position = 0;
			}
			return *this;
		}

		~file() {
			close();
		}

		auto get_os_handle() const {
			return handle;
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
			}
			else if (flags & O_TRUNC) {
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

			if (flags & O_ATE) {
				SetFilePointer(handle, 0, nullptr, FILE_END);
				file_position = size();
			}
			else {
				file_position = 0;
			}

			eof_flag = false;
			return true;
		}

		void close() {
			if (handle != INVALID_HANDLE_VALUE) {
				FlushFileBuffers(handle);
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

		std::int64_t read(std::span<char> buf) {
			if (handle == INVALID_HANDLE_VALUE) {
				throw std::system_error(std::make_error_code(std::errc::bad_file_descriptor));
			}

			DWORD bytes_read = 0;
			if (!ReadFile(handle, buf.data(), static_cast<DWORD>(buf.size()), &bytes_read, nullptr)) {
				throw std::system_error(std::make_error_code(std::errc::io_error));
			}

			if (bytes_read == 0) {
				eof_flag = true;
			}
			else {
				file_position += bytes_read;
			}

			return static_cast<std::int64_t>(bytes_read);
		}

		task<std::int64_t> read_async(Span<char> auto buf) {
			if (handle == INVALID_HANDLE_VALUE) {
				throw std::system_error(std::make_error_code(std::errc::bad_file_descriptor));
			}

			co_return co_await async_io_awaiter<char, ReadFile>(handle, buf, &file_position);
		}

		task<std::int64_t> write_async(Span<const char> auto buf) {
			if (handle == INVALID_HANDLE_VALUE) {
				throw std::system_error(std::make_error_code(std::errc::bad_file_descriptor));
			}

			co_return co_await async_io_awaiter<const char, WriteFile>(handle, buf, &file_position);
		}

		string read_line() {
			if (handle == INVALID_HANDLE_VALUE) {
				throw std::system_error(std::make_error_code(std::errc::bad_file_descriptor));
			}

			StringBuilder result;
			char buffer[256];

			while (true) {
				DWORD bytes_read = 0;
				if (!ReadFile(handle, buffer, sizeof(buffer), &bytes_read, nullptr)) {
					if (result.empty()) {
						throw std::system_error(std::make_error_code(std::errc::io_error), get_last_std_error());
					}
					break;
				}

				if (bytes_read == 0) {
					eof_flag = true;
					break;
				}

				for (DWORD i = 0; i < bytes_read; ++i) {
					if (buffer[i] == '\n') {
						if (i > 0 && buffer[i - 1] == '\r')
							result.pop_back();

						LONG move_back = static_cast<LONG>(bytes_read - i - 1);
						if (move_back > 0) {
							SetFilePointer(handle, -move_back, nullptr, FILE_CURRENT);
						}
						return result;
					}
					result.push_back(buffer[i]);
				}
			}

			return result;
		}

		std::int64_t write(Span<const char> auto buf) {
			if (handle == INVALID_HANDLE_VALUE) {
				throw std::system_error(std::make_error_code(std::errc::bad_file_descriptor));
			}

			DWORD bytes_written = 0;
			if (!WriteFile(handle, buf.data(), static_cast<DWORD>(buf.size()), &bytes_written, nullptr)) {
				throw std::system_error(std::make_error_code(std::errc::io_error));
			}

			file_position += bytes_written;
			return static_cast<std::int64_t>(bytes_written);
		}

		std::int64_t write_line(string line) {
			auto written = write(std::span<const char>(line.c_str(), line.size_bytes()));
			auto newline_result = write(std::span<const char>("\n", 1));
			return written + newline_result;
		}
	};

	static_assert(Reader<file>);
	static_assert(LineReader<file>);
	static_assert(AsyncReader<file>);
	static_assert(Writer<file>);
	static_assert(LineWriter<file>);
	static_assert(AsyncWriter<file>);

	file open(string path) {
		return { path };
	}

	file open(string path, int flags) {
		return { path, flags };
	}
};
