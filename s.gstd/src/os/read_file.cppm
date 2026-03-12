export module gs:read_file;
import std;
import :file;

namespace os {
	export std::vector<char> read_file(std::string_view filename) {
		if (os::file f(filename); f) {
			std::vector<char> data(f.size());
			auto bytes_read = f.read(data);
			if (static_cast<std::size_t>(bytes_read) == data.size())
				return data;
		}

		return {};
	}
}
