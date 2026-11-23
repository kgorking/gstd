export module gs:LineReader;
import std;
import :string;

export template<typename I>
concept LineReader = requires(I i) {
    { i.read_line() } -> std::same_as<string>;
};
