export module gs:read_text;
import std;
import :file;
import :string;

namespace os {
	export string read_text(string const& filename) {
		string text;
		std::vector<char> buffer(4096);
		
		file f = open(filename);
		while (std::int64_t r = f.read(buffer)) {
			if (!r) break;
			text = string::fmt("{}{}", text, std::string_view(buffer.data(), r));
		}

		return text;
	}
}
