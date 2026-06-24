export module gs:Reader;
import std;
import :co;

export template<typename I>
concept Reader = requires(I i, std::span<char> buf) {
	{ i.read(buf) } -> std::same_as<std::int64_t>;
};

export template<typename I>
concept AsyncReader = requires(I i, std::span<char> buf) {
	{ i.read_async(buf) } -> std::same_as<co<std::int64_t>>;
};
