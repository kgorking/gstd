export module gs:Writer;
import std;

export template<typename O>
concept Writer = requires(O o, std::span<const char> data) {
    { o.write(data) } -> std::same_as<std::expected<std::int64_t, std::error_code>>;
};
