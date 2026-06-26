module;
#include <windows.h>
#include <winhttp.h>

export module gs:http_get;
import :http;
import :string;
import std;

export namespace http {
	string get(string url) {
		auto rh = open_request(url, L"GET");

		if (!WinHttpSendRequest(rh.hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
			WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
			throw std::system_error(GetLastError(), std::system_category(), "WinHttpSendRequest failed");
		}

		if (!WinHttpReceiveResponse(rh.hRequest, nullptr)) {
			throw std::system_error(GetLastError(), std::system_category(), "WinHttpReceiveResponse failed");
		}

		return read_response(rh.hRequest);
	}
}
