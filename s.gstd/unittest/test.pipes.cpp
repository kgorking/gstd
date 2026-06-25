import gs;
import std;

test pipes_creation_and_validity = [] {
	auto p = io::pipes();
	test::is_true(p.reader, "reader should be valid");
	test::is_true(p.writer, "writer should be valid");
};

test pipes_write_and_read_small_data = [] {
	auto p = io::pipes();

	const char test_data[] = "Hello";
	auto write_result = p.writer.write(std::span<const char>(test_data, 5));
	test::equals(write_result, 5Z, "write should return 5 bytes");

	std::vector<char> buffer(32);
	auto read_result = p.reader.read(buffer);
	test::equals(read_result, 5Z, "read should return 5 bytes");
	bool b = std::memcmp(buffer.data(), "Hello", 5) == 0;
	test::is_true(b, "buffer content should match");
};

test pipes_reader_concept = [] {
	auto p = io::pipes();

	string const data = "test";
	auto w = p.writer.write(data);
	test::equals(w, 4Z, "write should return 4 bytes");

	// Verify reader satisfies Reader concept
	static_assert(Reader<std::remove_reference_t<decltype(p.reader)>>);

	std::vector<char> buf(32);
	auto result = p.reader.read(buf);
	test::equals(result, 4Z, "read should return 4 bytes");
};

test pipes_writer_concept = [] {
	auto p = io::pipes();

	// Verify writer satisfies Writer concept
	static_assert(Writer<std::remove_reference_t<decltype(p.writer)>>);

	string data = "test";
	auto result = p.writer.write_line(data);
	auto data_read_back = p.reader.read_line();
	test::equals(result, 5Z, "write should return 5 bytes");
	test::equals(data_read_back, data, "read_back content should match");
};

test pipes_line_reader_concept = [] {
	auto p = io::pipes();

	// Verify reader satisfies LineReader concept
	static_assert(LineReader<std::remove_reference_t<decltype(p.reader)>>);

	string line_data = "test line\n";
	auto write_result = p.writer.write(line_data);
	test::equals(write_result, 10Z, "write should return 10 bytes");

	auto line_result = p.reader.read_line();
	test::equals(line_result, string("test line"), "line content should match");
};

test pipes_line_writer_concept = [] {
	auto p = io::pipes();

	// Verify writer satisfies LineWriter concept
	static_assert(LineWriter<std::remove_reference_t<decltype(p.writer)>>);

	string test_line = "test line";
	auto result = p.writer.write_line(test_line);
	string test_line_read_back = p.reader.read_line();
	test::is_true(result >= 9, "write_line should write at least 9 bytes");
	test::equals(test_line_read_back, test_line, "read_back line content should match");
};
