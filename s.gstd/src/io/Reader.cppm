export module gs:Reader;
import std;

export template<typename I>
concept Reader = requires(I i, std::span<char> buf) {
	{ i.read(buf) } -> std::same_as<std::int64_t>;
};
