export module gs:read_text;
import std;
import :types;
import :file;
import :string;
import :fmt;

namespace io {
	export string read_text(string const& filename) {
		string text;
		std::vector<char> buffer(4096);
		
		file f = open(filename);
		while (int64 r = f.read(buffer)) {
			if (!r) break;
			text = fmt("{}{}", text, std::string_view(buffer.data(), r));
		}

		return text;
	}
}
