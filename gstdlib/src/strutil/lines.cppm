export module gs:lines;
import std;
import :sequence;
import :string;

export sequence<string> lines(string lines) {
    auto newline_pos = lines.find('\n');
    while (newline_pos != -1) {
        co_yield lines.substr(0, newline_pos);
        lines.remove_prefix(newline_pos + 1);
        newline_pos = lines.find('\n');
    }
    co_yield lines;
}
