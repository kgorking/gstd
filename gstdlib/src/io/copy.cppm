export module gs:copy;
import :types;
import :Reader;
import :Writer;
import std;

namespace io {
	export int64 copy(Writer auto&& writer, Reader auto&& reader) {
		auto buffer = std::array<char, 4096>{};
		auto span = std::span<const char>(buffer);

		int64 total_written = 0;
		int64 bytes_read = 0;
		while ((bytes_read = reader.read(buffer)) > 0) {
			int64 bytes_written = writer.write(span.subspan(0, bytes_read));
			total_written += bytes_written;
		}

		return total_written;
	}
}
