export module gs:Writer;
import std;
import :task;

export template<typename O>
concept Writer = requires(O o, std::span<const char> data) {
    { o.write(data) } -> std::same_as<std::int64_t>;
};

export template<typename O>
concept AsyncWriter = requires(O o, std::span<const char> data) {
    { o.write_async(data) } -> std::same_as<task<std::int64_t>>;
};
