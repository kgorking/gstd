module;
#define NOMINMAX
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

export module gs:http;
import std;
import :string;
import :string_builder;
import :types;
import :Reader;
import :concepts;

namespace http {
	enum class method {
		Get,
		Post,
		Put,
		Delete,
		Patch,
		Head,
		Options
	};

	LPCWSTR method_to_string(method m) {
		switch (m) {
		case method::Get: return L"GET";
		case method::Post: return L"POST";
		case method::Put: return L"PUT";
		case method::Delete: return L"DELETE";
		case method::Patch: return L"PATCH";
		case method::Head: return L"HEAD";
		case method::Options: return L"OPTIONS";
		default: throw std::invalid_argument("Invalid HTTP method");
		}
	}

	struct request {
		HINTERNET hSession{};
		HINTERNET hConnect{};
		HINTERNET hRequest{};
		std::wstring url_w;
		std::wstring path;
		DWORD size = 0;
		bool is_https = false;

		request(string url) {
			URL_COMPONENTSW url_comp = open(url);

			path = L"/";
			if (url_comp.lpszUrlPath && url_comp.dwUrlPathLength > 0) {
				path = std::wstring(url_comp.lpszUrlPath, url_comp.dwUrlPathLength);
				if (url_comp.lpszExtraInfo && url_comp.dwExtraInfoLength > 0) {
					path += std::wstring(url_comp.lpszExtraInfo, url_comp.dwExtraInfoLength);
				}
			}

			is_https = (url_comp.nScheme == INTERNET_SCHEME_HTTPS);

			open_request(method::Get);

			if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
				throw std::system_error(GetLastError(), std::system_category(), "WinHttpSendRequest failed");

			receive_response();
		}

		request(string url, string body, string content_type = "application/x-www-form-urlencoded") {
			URL_COMPONENTSW url_comp = open(url);

			path = L"/";
			if (url_comp.lpszUrlPath && url_comp.dwUrlPathLength > 0) {
				path = std::wstring(url_comp.lpszUrlPath, url_comp.dwUrlPathLength);
				if (url_comp.lpszExtraInfo && url_comp.dwExtraInfoLength > 0) {
					path += std::wstring(url_comp.lpszExtraInfo, url_comp.dwExtraInfoLength);
				}
			}

			is_https = (url_comp.nScheme == INTERNET_SCHEME_HTTPS);

			open_request(method::Post);

			int ct_len = MultiByteToWideChar(CP_UTF8, 0, content_type.c_str(), static_cast<int>(content_type.size_bytes()), nullptr, 0);
			std::wstring ct_w(ct_len, L'\0');
			MultiByteToWideChar(CP_UTF8, 0, content_type.c_str(), static_cast<int>(content_type.size_bytes()), ct_w.data(), ct_len);
			std::wstring headers = L"Content-Type: " + ct_w;

			if (!WinHttpSendRequest(hRequest,
				headers.c_str(),
				static_cast<DWORD>(headers.size()),
				(void*)body.c_str(),
				static_cast<DWORD>(body.size_bytes()),
				static_cast<DWORD>(body.size_bytes()),
				0)) {
				throw std::system_error(GetLastError(), std::system_category(), "WinHttpSendRequest failed");
			}

			receive_response();
		}

		int64 read(Span<char> auto& data) {
			DWORD bytes_read = 0;
			if (!WinHttpReadData(hRequest, data.data(), static_cast<DWORD>(data.size()), &bytes_read))
				throw std::system_error(GetLastError(), std::system_category(), "WinHttpReadData failed");
			return bytes_read;
		}

		~request() {
			if (hRequest) WinHttpCloseHandle(hRequest);
			if (hConnect) WinHttpCloseHandle(hConnect);
			if (hSession) WinHttpCloseHandle(hSession);
		}

	private:
		URL_COMPONENTSW open(string url) {
			int url_len = MultiByteToWideChar(CP_UTF8, 0, url.c_str(), static_cast<int>(url.size_bytes()), nullptr, 0);
			url_w = std::wstring(url_len, L'\0');
			MultiByteToWideChar(CP_UTF8, 0, url.c_str(), static_cast<int>(url.size_bytes()), url_w.data(), url_len);

			URL_COMPONENTSW url_comp = { sizeof(URL_COMPONENTSW) };
			url_comp.dwHostNameLength = ~0ul;
			url_comp.dwUrlPathLength = ~0ul;
			url_comp.dwExtraInfoLength = ~0ul;
			url_comp.dwSchemeLength = ~0ul;

			if (!WinHttpCrackUrl(url_w.c_str(), static_cast<DWORD>(url_w.size()), 0, &url_comp)) {
				throw std::system_error(GetLastError(), std::system_category(), "Failed to parse URL");
			}

			hSession = WinHttpOpen(L"gstdlib/1.0",
				WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
				WINHTTP_NO_PROXY_NAME,
				WINHTTP_NO_PROXY_BYPASS, 0);
			if (!hSession) {
				throw std::system_error(GetLastError(), std::system_category(), "WinHttpOpen failed");
			}

			wchar_t wc = url_comp.lpszHostName[url_comp.dwHostNameLength];
			url_comp.lpszHostName[url_comp.dwHostNameLength] = L'\0'; // this is so stupid
			hConnect = WinHttpConnect(hSession, url_comp.lpszHostName, url_comp.nPort, 0);
			url_comp.lpszHostName[url_comp.dwHostNameLength] = wc;
			if (!hConnect) {
				throw std::system_error(GetLastError(), std::system_category(), "WinHttpConnect failed");
			}

			return url_comp;
		}

		void open_request(method m) {
			hRequest = WinHttpOpenRequest(hConnect, method_to_string(m), path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, is_https ? WINHTTP_FLAG_SECURE : 0);
			if (!hRequest) {
				throw std::system_error(GetLastError(), std::system_category(), "WinHttpOpenRequest failed");
			}
		}

		void receive_response() {
			if (!WinHttpReceiveResponse(hRequest, nullptr))
				throw std::system_error(GetLastError(), std::system_category(), "WinHttpReceiveResponse failed");

			if (!WinHttpQueryDataAvailable(hRequest, &size))
				throw std::system_error(GetLastError(), std::system_category(), "WinHttpQueryDataAvailable failed");
		}
	};
}
