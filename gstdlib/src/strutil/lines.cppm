export module gs:lines;
import std;
import :sequence;
import :string;

export sequence<string> split_lines(string input) {
    auto newline_pos = input.find('\n');
    while (newline_pos != -1) {
        co_yield input.substr(0, newline_pos);
        input.remove_prefix(newline_pos + 1);
        newline_pos = input.find('\n');
    }
    co_yield input;
}

export sequence<string> lines(string input) {
    return split_lines(std::move(input));
}
