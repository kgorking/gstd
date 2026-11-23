import gs;

test os_read_lines_test = [] {
	string const file_content("line1\nline2\nline3\nline4\nline5\nline6\nline7\nline8\nline9\nline10");
	auto const w = io::write_text("lines_test.txt", file_content);
	test::equals(w, 60Z, "write should return 60 bytes");

	int line_count = 0;
	for (string line : io::read_lines("lines_test.txt")) {
		line_count += 1;
		if (line_count > 10)
			break;

		auto result = fmt("line{}", line_count);
		test::equals(line, result, "line content should match");
	}

	test::equals(line_count, 10, "should read 10 lines");
};
