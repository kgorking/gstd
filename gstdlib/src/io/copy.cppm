export module gs:copy;
import :types;
import :Reader;
import :Writer;
import std;

export int64 copy(Writer auto&& writer, Reader auto&& reader) {
	std::vector<char> buffer(4096);
	int64 total_written = 0;

	int64 bytes_read;
	while ((bytes_read = reader.read(buffer)) > 0) {
		int64 bytes_written = writer.write({ buffer.begin(), static_cast<std::size_t>(bytes_read) });
		total_written += bytes_written;
	}

	return total_written;
}