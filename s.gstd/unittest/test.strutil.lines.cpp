#include "doctest.h"
import std;
import gs;

TEST_CASE("test.co") {
	constexpr auto wait = std::chrono::milliseconds(25);
	constexpr char text_arr[] = "Line 1\nLine 2\nLine 3\nLine 4\nLine 5\nLine 6\nLine 7\nLine 8\nLine 9\nLine 10";
	string text = text_arr;
	char buffer[32];

	for (int i = 1; string line : lines(text)) {
		auto result = std::format_to_n(buffer, 31, "Line {}", i);
		*result.out = 0;  // Null-terminate the formatted string
		CHECK(line == buffer);
		i += 1;
	}
}
