export module gs:LineWriter;
import std;
import :types;
import :string;

export template<typename O>
concept LineWriter = requires(O o, string line) {
    { o.write_line(line) } -> std::same_as<int64>;
};
