export module gs:strconv;
import std;
import :string;

namespace strconv {
    export template <std::integral T = std::int64_t>
    auto to_int(string const& s, T* out = nullptr) -> T {
        T value{};
        auto [ptr, ec] = std::from_chars(s.c_str(), s.c_str() + s.size(), value);
        if (ec != std::errc{})
            throw std::system_error(std::make_error_code(ec));
        if (out)
            *out = value;
        return value;
    };
}
