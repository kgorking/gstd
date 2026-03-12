#include "doctest.h"
import gs;
import std;

TEST_CASE("test.pipes.creation_and_validity") {
	auto p = os::pipes();
	CHECK(p.reader);
	CHECK(p.writer);
}

TEST_CASE("test.pipes.write_and_read_small_data") {
	auto p = os::pipes();
	
	const char test_data[] = "Hello";
	auto write_result = p.writer.write(std::span<const char>(test_data, 5));
	CHECK(write_result == 5);
	
	std::vector<char> buffer(32);
	auto read_result = p.reader.read(buffer);
	CHECK(read_result == 5);
	CHECK(std::string_view(buffer.data(), 5) == "Hello");
}

TEST_CASE("test.pipes.reader_concept") {
	auto p = os::pipes();
	
	string data = "test";
	auto w = p.writer.write(data);
	CHECK(w == 4);
	
	// Verify reader satisfies Reader concept
	static_assert(Reader<std::remove_reference_t<decltype(p.reader)>>);
	
	std::vector<char> buf(32);
	auto result = p.reader.read(buf);
	CHECK(result == 4);
}

TEST_CASE("test.pipes.writer_concept") {
	auto p = os::pipes();
	
	// Verify writer satisfies Writer concept
	static_assert(Writer<std::remove_reference_t<decltype(p.writer)>>);
	
	string data = "test";
	auto result = p.writer.write(data);
	CHECK(result == 4);
}

TEST_CASE("test.pipes.line_reader_concept") {
	auto p = os::pipes();
	
	// Verify reader satisfies LineReader concept
	static_assert(LineReader<std::remove_reference_t<decltype(p.reader)>>);
	
	string line_data = "test line\n";
	auto write_result = p.writer.write(line_data);
	CHECK(write_result == 10);
	
	auto line_result = p.reader.read_line();
	CHECK(line_result == "test line");
}

TEST_CASE("test.pipes.line_writer_concept") {
	auto p = os::pipes();
	
	// Verify writer satisfies LineWriter concept
	static_assert(LineWriter<std::remove_reference_t<decltype(p.writer)>>);
	
	string test_line = "test line";
	auto result = p.writer.write_line(test_line);
	// Should include line content + newline
	CHECK(result >= 9);
}
