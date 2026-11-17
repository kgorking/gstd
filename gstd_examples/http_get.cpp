import gs;
import std;
using namespace std::chrono_literals;

static void hello_http(http::server_request const& /*req*/, http::response_writer& rw) {
	auto file = http::get("https://raw.githubusercontent.com/kgorking/gstd/refs/heads/main/gstdlib/src/gstd.cppm");
	io::copy(rw, file);
};

void xmain() {
	http::handle_func("/test", hello_http);
	http::listen_and_serve(":8080");
}
