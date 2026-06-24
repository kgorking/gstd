module;
#include <windows.h>
export module gs:get_last_error;
import std;
import :string;

export string get_last_error() {
	char* err = strerror(errno);
	return err ? err : "Unknown error";
}

export std::string get_last_std_error() {
	char* err = strerror(errno);
	return err ? err : "Unknown error";
}
