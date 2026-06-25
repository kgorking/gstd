export module gs:read_file;
import std;
import :file;

namespace io {
	export std::vector<char> read_file(std::string_view filename) {
		if (io::file f(filename); f) {
			std::vector<char> data(f.size());
			auto bytes_read = f.read(data);
			if (static_cast<std::size_t>(bytes_read) == data.size())
				return data;
		}

		return {};
	}
}
