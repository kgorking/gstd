import gs;
import gs.testing;
import std;

test pipes_creation_and_validity = [] {
	auto p = os::pipes();
	test::assert(p.reader, "reader should be valid");
	test::assert(p.writer, "writer should be valid");
};

test pipes_write_and_read_small_data = [] {
	auto p = os::pipes();

	const char test_data[] = "Hello";
	auto write_result = p.writer.write(std::span<const char>(test_data, 5));
	test::assert_eq(write_result, 5UZ, "write should return 5 bytes");

	std::vector<char> buffer(32);
	auto read_result = p.reader.read(buffer);
	test::assert_eq(read_result, 5UZ, "read should return 5 bytes");
	bool b = std::memcmp(buffer.data(), "Hello", 5) == 0;
	test::assert(b, "buffer content should match");
};

test pipes_reader_concept = [] {
	auto p = os::pipes();

	string data = "test";
	auto w = p.writer.write(data);
	test::assert_eq(w, 4UZ, "write should return 4 bytes");

	// Verify reader satisfies Reader concept
	static_assert(Reader<std::remove_reference_t<decltype(p.reader)>>);

	std::vector<char> buf(32);
	auto result = p.reader.read(buf);
	test::assert_eq(result, 4UZ, "read should return 4 bytes");
};

test pipes_writer_concept = [] {
	auto p = os::pipes();

	// Verify writer satisfies Writer concept
	static_assert(Writer<std::remove_reference_t<decltype(p.writer)>>);

	string data = "test";
	auto result = p.writer.write(data);
	test::assert_eq(result, 4UZ, "write should return 4 bytes");
};

test pipes_line_reader_concept = [] {
	auto p = os::pipes();

	// Verify reader satisfies LineReader concept
	static_assert(LineReader<std::remove_reference_t<decltype(p.reader)>>);

	string line_data = "test line\n";
	auto write_result = p.writer.write(line_data);
	test::assert_eq(write_result, 10UZ, "write should return 10 bytes");

	auto line_result = p.reader.read_line();
	test::assert_eq(line_result, string("test line"), "line content should match");
};

test pipes_line_writer_concept = [] {
	auto p = os::pipes();

	// Verify writer satisfies LineWriter concept
	static_assert(LineWriter<std::remove_reference_t<decltype(p.writer)>>);

	string test_line = "test line";
	auto result = p.writer.write_line(test_line);
	test::assert(result >= 9, "write_line should write at least 9 bytes");
};
