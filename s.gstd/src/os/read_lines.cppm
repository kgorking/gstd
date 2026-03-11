export module gs:read_lines;
import std;
import :file;
import :sequence;
import :string;

namespace os {
	export sequence<string> read_lines(string filename) {
		auto f = os::open(filename);
		try {
			while (!f.end_of_file()) {
				co_yield f.read_line();
			}
		} catch (const std::system_error&) {
			// End of file or error - stop iteration
		}
	}
}
