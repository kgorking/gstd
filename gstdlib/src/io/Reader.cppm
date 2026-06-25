export module gs:Reader;
import std;
import :types;
import :task;

export template<typename I>
concept Reader = requires(I i, std::span<char> buf) {
	{ i.read(buf) } -> std::same_as<int64>;
};

export template<typename I>
concept AsyncReader = requires(I i, std::span<char> buf) {
	{ i.read_async(buf) } -> std::same_as<task<int64>>;
};
