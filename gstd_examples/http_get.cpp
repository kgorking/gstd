import gs;

auto hello_http = [](http::request const& /*req*/, http::response_writer& rw) {
	rw.write(http::get("https://raw.githubusercontent.com/kgorking/gstd/refs/heads/main/gstdlib/src/gstd.cppm"));
	};

int main() {
	http::handle_func("/test", hello_http);
	http::listen_and_serve("::1:8080");

	print(http::get("https://raw.githubusercontent.com/kgorking/gstd/refs/heads/main/gstdlib/src/gstd.cppm"));
}