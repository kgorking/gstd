module;
#define NOMINMAX
#define WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <http.h>
#pragma comment(lib, "httpapi.lib")

export module gs:http_server;
import :string;
import :string_builder;
import :concepts;
import :types;
import :fmt;
import std;
using namespace std::string_view_literals;

namespace {
	std::string_view status_reason(int code) {
		switch (code) {
		case 200: return "OK"sv;
		case 201: return "Created"sv;
		case 204: return "No Content"sv;
		case 301: return "Moved Permanently"sv;
		case 302: return "Found"sv;
		case 304: return "Not Modified"sv;
		case 400: return "Bad Request"sv;
		case 401: return "Unauthorized"sv;
		case 403: return "Forbidden"sv;
		case 404: return "Not Found"sv;
		case 405: return "Method Not Allowed"sv;
		case 500: return "Internal Server Error"sv;
		case 502: return "Bad Gateway"sv;
		case 503: return "Service Unavailable"sv;
		default: return "Unknown"sv;
		}
	}

	std::wstring to_wide(string s) {
		if (s.empty()) return {};
		int len = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
		std::wstring w(len, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), len);
		return w;
	}

	std::string to_utf8(std::wstring_view w) {
		if (w.empty()) return {};
		int len = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
		std::string s(len, '\0');
		WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), s.data(), len, nullptr, nullptr);
		return s;
	}

	std::string_view verb_name(HTTP_VERB verb) {
		switch (verb) {
		case HttpVerbGET:     return "GET";
		case HttpVerbPOST:    return "POST";
		case HttpVerbPUT:     return "PUT";
		case HttpVerbDELETE:  return "DELETE";
		case HttpVerbHEAD:    return "HEAD";
		case HttpVerbOPTIONS: return "OPTIONS";
		case HttpVerbTRACE:   return "TRACE";
		case HttpVerbCONNECT: return "CONNECT";
		//case HttpVerbPATCH:   return "PATCH";
		default:              return {};
		}
	}
}

namespace http {
	export struct server_request {
		string method;
		string path;
	};

	export class response_writer {
		int status_ = 200;
		::string_builder body_;
	public:
		void write_status(int s) { status_ = s; }
		int64 write(Span<char const> auto data) { body_.push_span(data); return data.size(); }
		void write(string s) { body_.push_span(s); }

		int status() const { return status_; }
		::string_builder const& body() const { return body_; }
		std::string_view view() const { return { body_.buffer, (std::size_t)body_.size }; }
	};

	export using handler_func = void(*)(server_request const&, response_writer&);
	struct handler_entry {
		string pattern;
		handler_func handler;
	};

	std::vector<handler_entry> g_handlers;

	export void handle_func(string pattern, handler_func h) {
		http::g_handlers.push_back({ std::move(pattern), h });
	}

	export void listen_and_serve(string addr) {
		HTTPAPI_VERSION ver = HTTPAPI_VERSION_2;

		ULONG ret = HttpInitialize(ver, HTTP_INITIALIZE_SERVER, nullptr);
		if (ret != NO_ERROR) return;

		HTTP_SERVER_SESSION_ID sessionId{};
		ret = HttpCreateServerSession(ver, &sessionId, 0);
		if (ret != NO_ERROR) {
			HttpTerminate(HTTP_INITIALIZE_SERVER, nullptr);
			return;
		}

		HANDLE hQueue = nullptr;
		ret = HttpCreateRequestQueue(ver, nullptr, nullptr, 0, &hQueue);
		if (ret != NO_ERROR) {
			HttpCloseServerSession(sessionId);
			HttpTerminate(HTTP_INITIALIZE_SERVER, nullptr);
			return;
		}

		HTTP_URL_GROUP_ID urlGroupId{};
		ret = HttpCreateUrlGroup(sessionId, &urlGroupId, 0);
		if (ret != NO_ERROR) {
			HttpCloseRequestQueue(hQueue);
			HttpCloseServerSession(sessionId);
			HttpTerminate(HTTP_INITIALIZE_SERVER, nullptr);
			return;
		}

		HTTP_BINDING_INFO binding{{ 1 }, hQueue};
		ret = HttpSetUrlGroupProperty(urlGroupId, HttpServerBindingProperty, &binding, sizeof(binding));
		if (ret != NO_ERROR) {
			HttpCloseUrlGroup(urlGroupId);
			HttpCloseRequestQueue(hQueue);
			HttpCloseServerSession(sessionId);
			HttpTerminate(HTTP_INITIALIZE_SERVER, nullptr);
			return;
		}

		string url;
		if (addr.find(':') != std::string::npos) {
			if (addr.starts_with(":"))
				url = fmt("http://+{}/", addr);
			else
				url = fmt("http://{}/", addr);
		}
		else {
			url = fmt("http://+:{}/", addr);
		}

		std::wstring urlW = to_wide(url);
		ret = HttpAddUrlToUrlGroup(urlGroupId, urlW.c_str(), 0, 0);
		if (ret != NO_ERROR) {
			HttpCloseUrlGroup(urlGroupId);
			HttpCloseRequestQueue(hQueue);
			HttpCloseServerSession(sessionId);
			HttpTerminate(HTTP_INITIALIZE_SERVER, nullptr);
			return;
		}

		std::vector<char> buf(sizeof(HTTP_REQUEST) + 4096);
		while (true) {
			ULONG bytesRead = 0;

			ret = HttpReceiveHttpRequest(hQueue, HTTP_NULL_ID, 0, (PHTTP_REQUEST)buf.data(), (ULONG)buf.size(), &bytesRead, nullptr);
			if (ret != NO_ERROR) {
				if (ret == ERROR_MORE_DATA)
					buf.resize(buf.size() * 2);
				continue;
			}

			auto* req = (PHTTP_REQUEST)buf.data();

			std::string_view methodStr;
			if (req->Verb != HttpVerbUnknown) {
				methodStr = verb_name(req->Verb);
			} else if (req->pUnknownVerb && req->UnknownVerbLength > 0) {
				methodStr = { req->pUnknownVerb, req->UnknownVerbLength };
			}

			std::string pathStr;
			if (req->CookedUrl.pAbsPath && req->CookedUrl.AbsPathLength > 0) {
				ULONG plen = req->CookedUrl.AbsPathLength;
				if (plen > 0) plen--;
				pathStr = to_utf8({ req->CookedUrl.pAbsPath, plen });
			} else if (req->RawUrlLength > 0 && req->pRawUrl) {
				pathStr = std::string(req->pRawUrl, req->RawUrlLength);
			}

			handler_func matched = nullptr;
			for (auto const& entry : g_handlers) {
				std::string_view pat(entry.pattern.data(), entry.pattern.size_bytes());
				if (pathStr == pat || pathStr.starts_with(pat)) {
					matched = entry.handler;
					break;
				}
			}

			HTTP_RESPONSE response = {};
			HTTP_DATA_CHUNK chunk = {};

			response.Headers.KnownHeaders[HttpHeaderContentType].pRawValue = "text/plain; charset=utf-8";
			response.Headers.KnownHeaders[HttpHeaderContentType].RawValueLength = (USHORT)(sizeof("text/plain; charset=utf-8") - 1);

			response_writer rw;
			if (matched) {
				server_request sreq{ string(methodStr), string(pathStr) };
				matched(sreq, rw);

				auto reason = status_reason(rw.status());
				response.StatusCode = (USHORT)rw.status();
				response.pReason = reason.data();
				response.ReasonLength = (USHORT)reason.size();

				if (rw.body().size > 0) {
					chunk.DataChunkType = HttpDataChunkFromMemory;
					chunk.FromMemory.pBuffer = (PVOID)rw.body().buffer;
					chunk.FromMemory.BufferLength = (ULONG)rw.body().size;
					response.EntityChunkCount = 1;
					response.pEntityChunks = &chunk;
				}
			} else {
				response.StatusCode = 404;
				response.pReason = "Not Found";
				response.ReasonLength = 9;
			}

			ULONG sent = 0;
			HttpSendHttpResponse(hQueue, req->RequestId, 0, &response, nullptr, &sent, nullptr, 0, nullptr, nullptr);
		}

		HttpRemoveUrlFromUrlGroup(urlGroupId, urlW.c_str(), 0);
		HttpCloseUrlGroup(urlGroupId);
		HttpCloseRequestQueue(hQueue);
		HttpCloseServerSession(sessionId);
		HttpTerminate(HTTP_INITIALIZE_SERVER, nullptr);
	}
}
