export module gs:string_reader;
import std;
import :string;
import :Reader;
import :LineReader;

namespace strutil {
    export class string_reader {
        string s;

    public:
        string_reader(string str) : s(std::move(str)) {}

        std::expected<std::int64_t, std::error_code> read(std::span<char> buf) {
            if (s.empty())
                return 0;

            std::size_t to_read = std::min(buf.size(), static_cast<std::size_t>(s.size()));
            std::memcpy(buf.data(), s.data(), to_read);
            s.remove_prefix(to_read);
            return static_cast<std::int64_t>(to_read);
        }

        std::expected<string, std::error_code> read_line(char delim = '\n') {
            std::ptrdiff_t end = s.find(delim);
            if (end == string::npos) {
                string line = s;
                s.clear();
                return line;
            } else {
                string line = s.substr(0, end);
                s.remove_prefix(end+1);
                return line;
            }
        }
    };
    static_assert(Reader<string_reader>);
    static_assert(LineReader<string_reader>);
}