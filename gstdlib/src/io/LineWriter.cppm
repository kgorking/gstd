export module gs:LineWriter;
import std;
import :string;

export template<typename O>
concept LineWriter = requires(O o, string line) {
    { o.write_line(line) } -> std::same_as<std::int64_t>;
};
