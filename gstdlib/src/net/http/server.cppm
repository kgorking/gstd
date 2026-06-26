export module gs:http_server;
import :socket;
import :listener;
import :string;
import :string_builder;
import :concepts;
import :types;
import std;

namespace {
	std::string_view status_reason(int code) {
		switch (code) {
		case 200: return "OK";
		case 201: return "Created";
		case 204: return "No Content";
		case 301: return "Moved Permanently";
		case 302: return "Found";
		case 304: return "Not Modified";
		case 400: return "Bad Request";
		case 401: return "Unauthorized";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 500: return "Internal Server Error";
		case 502: return "Bad Gateway";
		case 503: return "Service Unavailable";
		default: return "Unknown";
		}
	}

	std::pair<string, string> parse_start_line(std::string_view line) {
		auto first_space = line.find(' ');
		auto second_space = line.find(' ', first_space + 1);
		if (first_space == std::string_view::npos || second_space == std::string_view::npos) {
			return {string(), string()};
		}
		string method(line.substr(0, first_space));
		string path(line.substr(first_space + 1, second_space - first_space - 1));
		return {std::move(method), std::move(path)};
	}
}

namespace http {
	export struct request {
		string method;
		string path;
	};

	export class response_writer {
		int status_ = 200;
		string_builder body_;
	public:
		void write_status(int s) { status_ = s; }
		void write(Span<char const> auto data) { body_.push_span(data); }
		void write(string const& s) { body_.push_span(std::span<char const>(s.c_str(), s.size_bytes())); }
		int status() const { return status_; }
		string_builder const& body() const { return body_; }
	};

	export using handler_func = void(*)(request const&, response_writer&);

	struct handler_entry {
		string pattern;
		handler_func handler;
	};

	std::vector<handler_entry> g_handlers;

	void send_response(net::socket& client, response_writer const& rw) {
		int64 const body_size = rw.body().size;
		std::string body_data(rw.body().buffer, body_size);

		std::string status_line = "HTTP/1.1 " + std::to_string(rw.status()) + " " +
			std::string(status_reason(rw.status())) + "\r\n";
		client.write(std::span<const char>(status_line.data(), status_line.size()));

		std::string content_type_hdr = "Content-Type: text/plain; charset=utf-8\r\n";
		client.write(std::span<const char>(content_type_hdr.data(), content_type_hdr.size()));

		std::string content_length_hdr = "Content-Length: " + std::to_string(body_size) + "\r\n";
		client.write(std::span<const char>(content_length_hdr.data(), content_length_hdr.size()));

		std::string crlf = "\r\n";
		client.write(std::span<const char>(crlf.data(), crlf.size()));

		if (body_size > 0) {
			client.write(std::span<const char>(body_data.data(), body_data.size()));
		}
	}

	export void handle_func(string pattern, handler_func h) {
		http::g_handlers.push_back({ std::move(pattern), h });
	}

	export void listen_and_serve(string addr) {
		net::listener lis = net::listen("tcp6", addr);

		while (true) {
			net::socket client = lis.accept();
			if (!client.valid())
				continue;

			char buf[4096];
			int64 bytes_read = client.read(std::span<char>(buf, sizeof(buf) - 1));
			if (bytes_read <= 0) {
				continue;
			}
			buf[bytes_read] = 0;

			std::string_view request_data(buf, bytes_read);
			auto header_end = request_data.find("\r\n\r\n");
			if (header_end == std::string_view::npos) {
				continue;
			}

			auto first_line_end = request_data.find("\r\n");
			if (first_line_end == std::string_view::npos) {
				continue;
			}
			std::string_view start_line = request_data.substr(0, first_line_end);

			auto [method, path] = parse_start_line(start_line);

			handler_func matched = nullptr;
			for (auto const& entry : g_handlers) {
				if (path == entry.pattern || path.starts_with(entry.pattern)) {
					matched = entry.handler;
					break;
				}
			}

			if (!matched) {
				response_writer rw;
				rw.write_status(404);
				rw.write(string("Not Found"));
				send_response(client, rw);
				continue;
			}

			request req{ std::move(method), std::move(path) };
			response_writer rw;
			matched(req, rw);
			send_response(client, rw);
		}
	}
}