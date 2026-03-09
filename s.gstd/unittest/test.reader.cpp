import std;
import gs;

using namespace std;

struct MyWriter
{
	auto write(std::span<const char> data) -> std::expected<std::int64_t, std::error_code> {
		if (data.size() == 0)
			return 0;
		std::string_view const sv{data.data(), static_cast<std::size_t>(data.size())};
		std::print("{}", sv);
		return static_cast<std::int64_t>(data.size_bytes());
	}
};
static_assert(Writer<MyWriter>);

static auto copy(Writer auto&& writer, Reader auto&& reader) -> std::expected<std::int64_t, std::error_code> {
	std::vector<char> buffer(4096);
	std::int64_t total_written = 0;

	using R = std::expected<std::int64_t, std::error_code>;
	R result;
	do {
		result = reader.read(buffer)
			.and_then([&](std::int64_t r) { return writer.write({buffer.begin(), static_cast<std::size_t>(r)}); })
			.and_then([&](std::int64_t w) { total_written += w; return R{ w }; });
	} while (result.value_or(0));

	return total_written;
}

//int main() {
//	return !copy(MyWriter{}, os::open("CMakeCache.txt"));
//}
