export module gs:read_file;
import std;
import :file;

namespace os {
	export std::vector<char> read_file(std::string_view filename) {
		if (os::file f(filename); f) {
			std::vector<char> data(f.size());
			auto r = f.read(std::span(data));
			if (r && static_cast<std::size_t>(*r) == data.size())
				return data;
		}

		return {};
	}
}
