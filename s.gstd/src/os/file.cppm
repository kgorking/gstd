export module gs:file;
import std;
import :Reader;
import :LineReader;
import :Writer;
import :string;

export namespace os {
	constexpr int O_RD = std::ios::in; // read
	constexpr int O_WR = std::ios::out; // write
	constexpr int O_RDWR = O_RD | O_WR; // read/write
	constexpr int O_ATE = std::ios::ate; // open at end
	constexpr int O_APP = std::ios::app; // always write at end
	constexpr int O_TRUNC = std::ios::trunc; // truncate (open and discard contents)
	constexpr int O_BIN = std::ios::binary; // binary mode (don't translate newlines)
	constexpr int O_CREATE = O_WR | O_TRUNC;

	class file final {
		std::fstream f;

	public:
		file(string path) : f(path.c_str(), static_cast<std::ios_base::openmode>(O_RD | O_BIN)) {}
		file(string path, int flags) : f(path.c_str(), static_cast<std::ios_base::openmode>(flags)) {}
		~file() {
			if (f.is_open())
				f.close();
		}

		bool open(string path, int flags) {
            std::ios::openmode mode = static_cast<std::ios::openmode>(flags);
            f.open(path.c_str(), mode);
			return f.good();
		}

		void close() {
			f.close();
		}

		operator bool() const {
			return f.is_open();
		}

		bool end_of_file() const {
			return f.eof();
		}

		std::size_t size() {
			if (!f.is_open())
				return 0;

			auto current_pos = f.tellg();
			f.seekg(0, std::ios::end);
			auto size = f.tellg();
			f.seekg(current_pos);
			return size;
		}

		std::expected<std::int64_t, std::error_code> read(std::span<char> buf) {
			if (f.bad())
				return std::unexpected(std::make_error_code(std::errc::io_error));

			f.read(buf.data(), buf.size());
			return f.gcount();
		}

		std::expected<string, std::error_code> read_line() {
			if (f.bad())
				return std::unexpected(std::make_error_code(std::errc::io_error));

			std::string line;
			std::getline(f, line);
			return {line.c_str()};
		}

		std::expected<std::int64_t, std::error_code> write(std::span<const char> buf) {
			if (f.bad())
				return std::unexpected(std::make_error_code(std::errc::io_error));

			std::int64_t pos = f.tellg();
			f.write(buf.data(), buf.size());
			return f.tellg() - pos;
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
