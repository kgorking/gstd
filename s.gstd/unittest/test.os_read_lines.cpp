#include "doctest.h"
import gs;
import std;

TEST_CASE("test.os_read_lines") {
	auto const w = os::write_text("lines_test.txt", "line1\nline2\nline3\nline4\nline5\nline6\nline7\nline8\nline9\nline10");
	CHECK(w == 60);

	char buffer[32];
	int line_count = 0;
	for (string line : os::read_lines("lines_test.txt")) {
		line_count += 1;
		if (line_count > 10)
			break;

		std::memset(buffer, 0, 32);
		auto result = std::format_to_n(buffer, 31, "line{}", line_count);
		*result.out = 0;  // Null-terminate the formatted string
		CHECK(line == buffer);
	}

	CHECK(line_count == 10);
}
