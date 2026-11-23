module;
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#endif

export module gs:http_get;
import :http;
import :string;
import :Reader;
import std;

export namespace http {
	Reader auto get(string url) {
		return request(url);
	}
}
