export module gs:read_lines;
import std;
import :file;
import :sequence;
import :string;

namespace os {
	export sequence<string> read_lines(string filename) {
		auto f = os::open(filename);
		auto s = f.read_line();
		while(s && !f.end_of_file()) {
			co_yield *s;
			 s = f.read_line();
		}

		co_yield *s;
	}
}
