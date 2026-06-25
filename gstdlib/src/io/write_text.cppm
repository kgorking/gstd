export module gs:write_text;
import std;
import :file;
import :string;
import :concepts;
import :types;

namespace io {
	export auto write_text(string filename, Span<const char> auto text) -> int64 {
		return open(filename, O_WR | O_TRUNC | O_BIN).write(text);
	}

	export auto write_text_async(string filename, Span<const char> auto text) -> task<int64> {
		co_return co_await open(filename, O_WR | O_TRUNC | O_BIN).write_async(text);
	}
}
