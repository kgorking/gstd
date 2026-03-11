export module gs:write_text;
import std;
import :file;
import :string;

namespace os {
	export auto write_text(string filename, string text) -> std::int64_t {
		return open(filename, O_WR | O_TRUNC | O_BIN).write(std::span(text.c_str(), text.size()));
	}
}
