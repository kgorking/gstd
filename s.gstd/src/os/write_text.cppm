export module gs:write_text;
import std;
import :file;
import :string;

namespace os {
	export auto write_text(string filename, string text) -> std::int64_t {
		return open(filename, O_WR | O_TRUNC | O_BIN).write(std::span(text.data(), text.size()));
	}

	export auto write_text_async(string filename, string text) -> co<std::int64_t> {
		co_return co_await open(filename, O_WR | O_TRUNC | O_BIN).write_async(std::span(text.data(), text.size()));
	}
}
