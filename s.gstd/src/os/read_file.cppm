export module gs:read_file;
import std;
import :file;

namespace os {
	export std::vector<char> read_file(std::string_view filename) {
		if (os::file f(filename); f) {
			std::vector<char> data(f.size());
			try {
				auto bytes_read = f.read(std::span(data));
				if (static_cast<std::size_t>(bytes_read) == data.size())
					return data;
			} catch (const std::system_error&) {
				// Read error - return empty vector
			}
		}

		return {};
	}
}
