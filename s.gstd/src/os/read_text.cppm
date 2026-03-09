export module gs:read_text;
import std;
import :file;
import :string;

namespace os {
	export auto read_text(string const& filename) -> std::expected<std::string, std::error_code> {
		std::string text;
		std::vector<char> buffer(4096);
		
		file f = open(filename);
		while (auto r = f.read(buffer)) {
			if (!r) return std::unexpected(r.error());
			if (!*r) break;
			text.append(buffer.data(), *r);
		}

		return text;
	}
}
