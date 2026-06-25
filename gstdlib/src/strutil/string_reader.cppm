export module gs:string_reader;
import std;
import :types;
import :string;
import :Reader;
import :LineReader;

namespace strutil {
    export class string_reader {
        string s;

    public:
        string_reader(string str) : s(std::move(str)) {}

        int64 read(std::span<char> buf) {
            if (s.empty())
                return 0;

            int64 to_read = std::min(static_cast<int64>(buf.size()), static_cast<int64>(s.size()));
            std::memcpy(buf.data(), s.c_str(), to_read);
            s.remove_prefix(to_read);
            return static_cast<int64>(to_read);
        }

        string read_line(char delim = '\n') {
            int64 end = s.find(delim);
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