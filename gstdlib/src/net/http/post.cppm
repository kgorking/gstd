module;
#include <windows.h>
#include <winhttp.h>

export module gs:http_post;
import :http;
import :string;
import :Reader;
import std;

export namespace http {
	Reader auto post_form(string url, string body, string content_type = "application/x-www-form-urlencoded") {
		return request(url, body, content_type);
	}
}
