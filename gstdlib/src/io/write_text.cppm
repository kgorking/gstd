export module gs:write_text;
import std;
import :file;
import :string;
import :concepts;

namespace io {
	export auto write_text(string filename, Span<const char> auto text) -> std::int64_t {
		return open(filename, O_WR | O_TRUNC | O_BIN).write(text);
	}

	export auto write_text_async(string filename, Span<const char> auto text) -> task<std::int64_t> {
		co_return co_await open(filename, O_WR | O_TRUNC | O_BIN).write_async(text);
	}
}
