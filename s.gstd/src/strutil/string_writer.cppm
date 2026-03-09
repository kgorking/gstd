export module gs:string_writer;
import std;
import :string;
import :Writer;

namespace strutil {
    export class string_writer {
        std::string& s;

    public:
        string_writer(std::string& str) : s(str) {}

        std::expected<std::int64_t, std::error_code> write(std::span<const char> buf) {
            std::string_view const sv{buf.data(), static_cast<std::size_t>(buf.size())};
            s.append(sv.data(), sv.size());
            return static_cast<std::int64_t>(buf.size_bytes());
        }
    };
    static_assert(Writer<string_writer>);
}
