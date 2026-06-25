module;
#include <windows.h>
#include <winhttp.h>

export module gs:http_post;
import :http;
import :string;
import std;

export namespace http {
	string post(string url, string body, string content_type = "application/x-www-form-urlencoded") {
		auto rh = open_request(url, L"POST");

		int ct_len = MultiByteToWideChar(CP_UTF8, 0, content_type.c_str(), static_cast<int>(content_type.size_bytes()), nullptr, 0);
		std::wstring ct_w(ct_len, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, content_type.c_str(), static_cast<int>(content_type.size_bytes()), ct_w.data(), ct_len);
		std::wstring headers = L"Content-Type: " + ct_w;

		if (!WinHttpSendRequest(rh.request, headers.c_str(), static_cast<DWORD>(headers.size()),
			const_cast<char*>(body.c_str()), static_cast<DWORD>(body.size_bytes()),
			static_cast<DWORD>(body.size_bytes()), 0)) {
			throw std::system_error(GetLastError(), std::system_category(), "WinHttpSendRequest failed");
		}

		if (!WinHttpReceiveResponse(rh.request, nullptr)) {
			throw std::system_error(GetLastError(), std::system_category(), "WinHttpReceiveResponse failed");
		}

		return read_response(rh.request);
	}
}
