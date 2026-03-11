import std;
import gs;

using namespace std;

struct MyWriter
{
	auto write(std::span<const char> data) -> std::int64_t {
		if (data.size() == 0)
			return 0;
		std::string_view const sv{data.data(), static_cast<std::size_t>(data.size())};
		std::print("{}", sv);
		return static_cast<std::int64_t>(data.size_bytes());
	}
};
static_assert(Writer<MyWriter>);

static auto copy(Writer auto&& writer, Reader auto&& reader) -> std::int64_t {
	std::vector<char> buffer(4096);
	std::int64_t total_written = 0;

	std::int64_t bytes_read;
	while ((bytes_read = reader.read(buffer)) > 0) {
		std::int64_t bytes_written = writer.write({buffer.begin(), static_cast<std::size_t>(bytes_read)});
		total_written += bytes_written;
	}

	return total_written;
}

//int main() {
//	return !copy(MyWriter{}, os::open("CMakeCache.txt"));
//}
