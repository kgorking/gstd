module;
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

export module gs:http;
import std;
import :string;
import :string_builder;
import :types;

struct request_handle {
	HINTERNET session;
	HINTERNET connect;
	HINTERNET request;

	~request_handle() {
		WinHttpCloseHandle(request);
		WinHttpCloseHandle(connect);
		WinHttpCloseHandle(session);
	}
};

request_handle open_request(string url, wchar_t const* method) {
	int url_len = MultiByteToWideChar(CP_UTF8, 0, url.c_str(), static_cast<int>(url.size_bytes()), nullptr, 0);
	std::wstring url_w(url_len, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, url.c_str(), static_cast<int>(url.size_bytes()), url_w.data(), url_len);

	URL_COMPONENTSW url_comp = { sizeof(URL_COMPONENTSW) };
	url_comp.dwHostNameLength = static_cast<DWORD>(-1);
	url_comp.dwUrlPathLength = static_cast<DWORD>(-1);
	url_comp.dwExtraInfoLength = static_cast<DWORD>(-1);
	url_comp.dwSchemeLength = static_cast<DWORD>(-1);

	if (!WinHttpCrackUrl(url_w.c_str(), static_cast<DWORD>(url_w.size()), 0, &url_comp)) {
		throw std::system_error(GetLastError(), std::system_category(), "Failed to parse URL");
	}

	bool const is_https = (url_comp.nScheme == INTERNET_SCHEME_HTTPS);
	INTERNET_PORT const port = url_comp.nPort;

	std::wstring hostname(url_comp.lpszHostName, url_comp.dwHostNameLength);

	std::wstring path = L"/";
	if (url_comp.lpszUrlPath && url_comp.dwUrlPathLength > 0) {
		path = std::wstring(url_comp.lpszUrlPath, url_comp.dwUrlPathLength);
		if (url_comp.lpszExtraInfo && url_comp.dwExtraInfoLength > 0) {
			path += std::wstring(url_comp.lpszExtraInfo, url_comp.dwExtraInfoLength);
		}
	}

	HINTERNET const hSession = WinHttpOpen(L"gstdlib/1.0",
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession) {
		throw std::system_error(GetLastError(), std::system_category(), "WinHttpOpen failed");
	}

	HINTERNET const hConnect = WinHttpConnect(hSession, hostname.c_str(), port, 0);
	if (!hConnect) {
		DWORD const err = GetLastError();
		WinHttpCloseHandle(hSession);
		throw std::system_error(err, std::system_category(), "WinHttpConnect failed");
	}

	HINTERNET const hRequest = WinHttpOpenRequest(hConnect, method, path.c_str(),
		nullptr, WINHTTP_NO_REFERER,
		WINHTTP_DEFAULT_ACCEPT_TYPES,
		is_https ? WINHTTP_FLAG_SECURE : 0);
	if (!hRequest) {
		DWORD const err = GetLastError();
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		throw std::system_error(err, std::system_category(), "WinHttpOpenRequest failed");
	}

	return { hSession, hConnect, hRequest };
}

string read_response(HINTERNET hRequest) {
	string_builder builder;
	char buffer[4096];
	DWORD bytes_read = 0;

	while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytes_read) && bytes_read > 0) {
		builder.push_span(std::span<char const>(buffer, bytes_read));
	}

	return string(std::move(builder));
}
