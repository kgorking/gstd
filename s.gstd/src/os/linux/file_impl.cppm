module;
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
export module gs:file_impl;
import std;
import :Reader;
import :LineReader;
import :Writer;
import :LineWriter;
import :string;

export namespace os {
	constexpr int O_RD = O_RDONLY; // read
	constexpr int O_WR = O_WRONLY; // write
	constexpr int O_RDWR = 0x0003; // read/write (O_RDONLY | O_WRONLY)
	constexpr int O_ATE = 0x0004; // open at end (custom flag)
	constexpr int O_APP = O_APPEND; // always write at end
	constexpr int O_TRUNC = O_TRUNC; // truncate (open and discard contents)
	constexpr int O_BIN = 0x0008; // binary mode (custom flag, no-op on Linux)
	constexpr int O_CREATE = O_WRONLY | O_CREAT | O_TRUNC;

	class file final {
		int fd = -1;
		bool eof_flag = false;

	public:
		// Constructor for opening files by path
		file(string path) {
			open(path, O_RD | O_BIN);
		}

		file(string path, int flags) {
			open(path, flags);
		}

		// Constructor for pipes and other file descriptors
		explicit file(int f) : fd(f), eof_flag(false) {}

		file(const file&) = delete;
		file& operator=(const file&) = delete;

		file(file&& other) noexcept : fd(other.fd), eof_flag(other.eof_flag) {
			other.fd = -1;
			other.eof_flag = false;
		}

		file& operator=(file&& other) noexcept {
			if (this != &other) {
				close();
				fd = other.fd;
				eof_flag = other.eof_flag;
				other.fd = -1;
				other.eof_flag = false;
			}
			return *this;
		}

		~file() {
			close();
		}

		bool open(string path, int flags) {
			close();

			int posix_flags = 0;

			// Handle custom flags
			if ((flags & O_RD) && (flags & O_WR)) {
				posix_flags |= O_RDWR;
			} else if (flags & O_WR) {
				posix_flags |= O_WRONLY;
			} else {
				posix_flags |= O_RDONLY;
			}

			if (flags & O_APP) {
				posix_flags |= O_APPEND;
			}
			if (flags & O_TRUNC) {
				posix_flags |= O_TRUNC;
			}
			if (flags & O_CREATE) {
				posix_flags |= O_CREAT;
			}

			fd = ::open(path.c_str(), posix_flags, 0644);

			if (fd < 0) {
				return false;
			}

			// If O_ATE flag is set, seek to end
			if (flags & O_ATE) {
				lseek(fd, 0, SEEK_END);
			}

			eof_flag = false;
			return true;
		}

		void close() {
			if (fd >= 0) {
				::close(fd);
				fd = -1;
				eof_flag = false;
			}
		}

		operator bool() const {
			return fd >= 0;
		}

		bool end_of_file() const {
			return eof_flag;
		}

		std::size_t size() {
			if (fd < 0)
				return 0;

			off_t current_pos = lseek(fd, 0, SEEK_CUR);
			off_t end_pos = lseek(fd, 0, SEEK_END);
			lseek(fd, current_pos, SEEK_SET);

			if (end_pos < 0)
				return 0;

			return static_cast<std::size_t>(end_pos);
		}

		std::int64_t read(std::span<char> buf) {
			if (fd < 0) {
				throw std::system_error(std::make_error_code(std::errc::bad_file_descriptor));
			}

			ssize_t bytes_read = ::read(fd, buf.data(), buf.size());

			if (bytes_read < 0) {
				throw std::system_error(std::make_error_code(std::errc::io_error));
			}

			if (bytes_read == 0) {
				eof_flag = true;
			}

			return static_cast<std::int64_t>(bytes_read);
		}

		string read_line() {
			if (fd < 0) {
				throw std::system_error(std::make_error_code(std::errc::bad_file_descriptor));
			}

			std::string result;
			char buffer[256];

			while (true) {
				ssize_t bytes_read = ::read(fd, buffer, sizeof(buffer));

				if (bytes_read < 0) {
					if (result.empty()) {
						throw std::system_error(std::make_error_code(std::errc::io_error));
					}
					break;
				}

				if (bytes_read == 0) {
					eof_flag = true;
					break;
				}

				for (ssize_t i = 0; i < bytes_read; ++i) {
					if (buffer[i] == '\n') {
						// Found newline - don't include it, move file pointer back to after the newline
						ssize_t move_back = bytes_read - i - 1;
						if (move_back > 0) {
							lseek(fd, -move_back, SEEK_CUR);
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

		std::int64_t write(std::span<const char> buf) {
			if (fd < 0) {
				throw std::system_error(std::make_error_code(std::errc::bad_file_descriptor));
			}

			ssize_t bytes_written = ::write(fd, buf.data(), buf.size());

			if (bytes_written < 0) {
				throw std::system_error(std::make_error_code(std::errc::io_error));
			}

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
	static_assert(Writer<file>);
	static_assert(LineWriter<file>);

	file open(string path) {
		return { path };
	}

	file open(string path, int flags) {
		return { path, flags };
	}
};
