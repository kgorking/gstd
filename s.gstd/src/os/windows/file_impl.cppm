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
import :co;
import :io_scheduler;

export namespace os {	// Async read awaiter
	class async_read_awaiter {
	private:
		HANDLE file_handle;
		std::span<char> buffer;
		OVERLAPPED* overlapped;
		ULONGLONG* file_position;  // Pointer to file's position tracker

	public:
		async_read_awaiter(HANDLE h, Span<char> auto& buf, ULONGLONG* pos) 
			: file_handle(h), buffer(buf), overlapped(nullptr), file_position(pos) {}

		bool await_ready() const { return false; }  // Always suspend

		void await_suspend(std::coroutine_handle<> cont) {
			overlapped = get_scheduler().register_operation(cont);
			
			// Set the file position in the OVERLAPPED structure
			overlapped->Offset = static_cast<DWORD>(*file_position & 0xFFFFFFFF);
			overlapped->OffsetHigh = static_cast<DWORD>((*file_position >> 32) & 0xFFFFFFFF);

			DWORD bytes_read = 0;
			BOOL success = ReadFile(
				file_handle,
				buffer.data(),
				static_cast<DWORD>(buffer.size()),
				&bytes_read,
				overlapped
			);

			if (success) {
				// Synchronous completion - bytes_read is valid, set result only (position updated in await_resume)
				get_scheduler().set_operation_result(overlapped, static_cast<std::int64_t>(bytes_read), false);
				cont.resume();
			} else if (GetLastError() != ERROR_IO_PENDING) {
				// Error occurred
				get_scheduler().set_operation_result(overlapped, 0, true);
				cont.resume();
			}
			// If ERROR_IO_PENDING, await_resume will update position when result arrives
		}

		std::int64_t await_resume() {
			auto [res, err] = get_scheduler().get_operation_result(overlapped);
			if (overlapped) delete overlapped;
			
			if (err) {
				throw std::system_error(std::make_error_code(std::errc::io_error));
			}
			
			// Update file position based on bytes read
			*file_position += res;
			return res;
		}
	};

	// Async write awaiter
	class async_write_awaiter {
	private:
		HANDLE file_handle;
		std::span<const char> buffer;
		OVERLAPPED* overlapped;
		ULONGLONG* file_position;  // Pointer to file's position tracker

	public:
		async_write_awaiter(HANDLE h, Span<const char> auto const& buf, ULONGLONG* pos) 
			: file_handle(h), buffer(buf), overlapped(nullptr), file_position(pos) {}

		bool await_ready() const { return false; }

		void await_suspend(std::coroutine_handle<> cont) {
			overlapped = get_scheduler().register_operation(cont);
			
			// Set the file position in the OVERLAPPED structure
			overlapped->Offset = static_cast<DWORD>(*file_position & 0xFFFFFFFF);
			overlapped->OffsetHigh = static_cast<DWORD>((*file_position >> 32) & 0xFFFFFFFF);

			DWORD bytes_written = 0;
			BOOL success = WriteFile(
				file_handle,
				buffer.data(),
				static_cast<DWORD>(buffer.size()),
				&bytes_written,
				overlapped
			);

			if (success) {
				// Synchronous completion - bytes_written is valid, set result only (position updated in await_resume)
				get_scheduler().set_operation_result(overlapped, static_cast<std::int64_t>(bytes_written), false);
				cont.resume();
			} else if (GetLastError() != ERROR_IO_PENDING) {
				// Error occurred
				get_scheduler().set_operation_result(overlapped, 0, true);
				cont.resume();
			}
			// If ERROR_IO_PENDING, await_resume will update position when result arrives
		}

		std::int64_t await_resume() {
			auto [res, err] = get_scheduler().get_operation_result(overlapped);
			if (overlapped) delete overlapped;
			
			if (err) {
				throw std::system_error(std::make_error_code(std::errc::io_error));
			}
			
			// Update file position based on bytes written
			*file_position += res;
			return res;
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
		ULONGLONG file_position = 0;  // Track current file position for OVERLAPPED I/O
		bool is_real_file = false;  // Track if this is a real file (not a pipe or other device)

	public:
		// Constructor for opening files by path
		file(string path) {
			handle = INVALID_HANDLE_VALUE;
			eof_flag = false;
			is_real_file = false;
			open(path, O_RD | O_BIN);
			if (handle != INVALID_HANDLE_VALUE) {
				is_real_file = true;
				get_scheduler().associate_handle(handle);
			}
		}

		file(string path, int flags) {
			handle = INVALID_HANDLE_VALUE;
			eof_flag = false;
			is_real_file = false;
			open(path, flags);
			if (handle != INVALID_HANDLE_VALUE) {
				is_real_file = true;
				get_scheduler().associate_handle(handle);
			}
		}

		// Constructor for pipes and other handles
		// skip_iocp=true to skip IOCP registration (useful for pipes which use synchronous I/O)
		explicit file(HANDLE h, bool skip_iocp = false) : handle(h), eof_flag(false) {
			if (handle != INVALID_HANDLE_VALUE && !skip_iocp) {
				get_scheduler().associate_handle(handle);
			}
		}

		file(const file&) = delete;
		file& operator=(const file&) = delete;

		file(file&& other) noexcept : handle(other.handle), eof_flag(other.eof_flag), is_real_file(other.is_real_file) {
			other.handle = INVALID_HANDLE_VALUE;
			other.eof_flag = false;
		}

		file& operator=(file&& other) noexcept {
			if (this != &other) {
				close();
				handle = other.handle;
				eof_flag = other.eof_flag;
				is_real_file = other.is_real_file;
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
				file_position = size();
			} else {
				file_position = 0;
			}

			eof_flag = false;
			return true;
		}

		void close() {
			if (handle != INVALID_HANDLE_VALUE) {
				// Only flush for real files, not pipes or other devices
				if (is_real_file) {
					FlushFileBuffers(handle);
				}
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

		std::int64_t read(Span<char> auto& buf) {
			if (handle == INVALID_HANDLE_VALUE) {
				throw std::system_error(std::make_error_code(std::errc::bad_file_descriptor));
			}

			DWORD bytes_read = 0;
			if (!ReadFile(handle, buf.data(), static_cast<DWORD>(buf.size()), &bytes_read, nullptr)) {
				throw std::system_error(std::make_error_code(std::errc::io_error));
			}

			if (bytes_read == 0) {
				eof_flag = true;
			} else {
				file_position += bytes_read;
			}

			return static_cast<std::int64_t>(bytes_read);
		}

		co<std::int64_t> read_async(Span<char> auto& buf) {
			if (handle == INVALID_HANDLE_VALUE) {
				throw std::system_error(std::make_error_code(std::errc::bad_file_descriptor));
			}
			
			co_return co_await async_read_awaiter(handle, buf, &file_position);
		}

		co<std::int64_t> write_async(Span<const char> auto const& buf) {
			if (handle == INVALID_HANDLE_VALUE) {
				throw std::system_error(std::make_error_code(std::errc::bad_file_descriptor));
			}
			
			co_return co_await async_write_awaiter(handle, buf, &file_position);
		}

		string read_line() {
			if (handle == INVALID_HANDLE_VALUE) {
				throw std::system_error(std::make_error_code(std::errc::bad_file_descriptor));
			}

			std::string result;
			char buffer[256];

			while (true) {
				DWORD bytes_read = 0;
				if (!ReadFile(handle, buffer, sizeof(buffer), &bytes_read, nullptr)) {
					if (result.empty()) {
						throw std::system_error(std::make_error_code(std::errc::io_error));
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

			throw std::system_error(std::make_error_code(std::errc::io_error));
		}

		std::int64_t write(Span<const char> auto const& buf) {
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
