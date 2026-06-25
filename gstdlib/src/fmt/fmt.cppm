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

