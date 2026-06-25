export module gs:fmt;
import :string_builder;
import :string;
import std;

// Format a string
export template<typename... Args>
string fmt(std::format_string<Args...> format_str, Args&&... args) {
	string_builder builder;
	std::format_to(std::back_inserter(builder), format_str, std::forward<Args>(args)...);
	return string(std::move(builder));
}

export template<typename... Args>
void print(std::format_string<Args...> format_str, Args&&... args) {
	std::print(format_str, std::forward<Args>(args)...);
}

export template<typename... Args>
void print(Args&&... args) {
	std::string format_str;
	for(int i=0; i<sizeof...(Args); ++i) {
		format_str += "{}";
	}
	std::cout << std::vformat(format_str, std::make_format_args(args...));
}


export template<typename... Args>
void println(std::format_string<Args...> format_str, Args&&... args) {
	std::println(format_str, std::forward<Args>(args)...);
}

