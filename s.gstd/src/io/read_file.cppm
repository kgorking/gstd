export module gs:read_file;
import std;
import :types;
import :file;

namespace io {
	export std::vector<char> read_file(std::string_view filename) {
		if (io::file f(filename); f) {
			std::vector<char> data(f.size());
			auto bytes_read = f.read(data);
			if (bytes_read == std::ssize(data))
				return data;
		}

		return {};
	}
}
