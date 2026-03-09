export module gs:Reader;
import std;

export template<typename I>
concept Reader = requires(I i, std::span<char> buf) {
	{ i.read(buf) } -> std::same_as<std::expected<std::int64_t, std::error_code>>;
};
