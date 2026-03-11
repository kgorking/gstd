export module gs:read_text;
import std;
import :file;
import :string;

namespace os {
	export auto read_text(string const& filename) -> std::string {
		std::string text;
		std::vector<char> buffer(4096);
		
		file f = open(filename);
		while (std::int64_t r = f.read(buffer)) {
			if (!r) break;
			text.append(buffer.data(), r);
		}

		return text;
	}
}
