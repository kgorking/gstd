import gs;
import gs.testing;

test os_read_lines_test = [] {
	string const file_content("line1\nline2\nline3\nline4\nline5\nline6\nline7\nline8\nline9\nline10");
	auto const w = os::write_text("lines_test.txt", file_content);
	test::assert_eq(w, 60UZ, "write should return 60 bytes");

	int line_count = 0;
	for (string line : os::read_lines("lines_test.txt")) {
		line_count += 1;
		if (line_count > 10)
			break;

		auto result = string::fmt("line{}", line_count);
		test::assert_eq(line, result, "line content should match");
	}

	test::assert_eq(line_count, 10, "should read 10 lines");
};
