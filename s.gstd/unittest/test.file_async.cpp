#include "doctest.h"
import std;
import gs;

// Test coroutine for async read
static co<std::int64_t> test_async_read_impl(os::file& f, std::span<char> buf) {
	co_return co_await f.read_async(buf);
}

// Test coroutine for async write
static co<std::int64_t> test_async_write_impl(os::file& f, std::span<const char> buf) {
	co_return co_await f.write_async(buf);
}

TEST_CASE("file async read") {
	// Create a temporary test file
	string test_file = "test_async_read.txt";
	string test_content = "Hello, async world!";
	
	// Write test content using synchronous write
	os::write_text(test_file, test_content);
	
	// Now test async read - keep file alive across the async operation
	os::file f{test_file};
	char buf[256] = {0};
	
	auto result = test_async_read_impl(f, std::span(buf)).wait().result();
	
	CHECK(result == test_content.size());
	CHECK(std::string_view(buf, result) == test_content);
	
	f.close();
	
	// Clean up
	std::remove(test_file.c_str());
}

TEST_CASE("file async write") {
	string test_file = "test_async_write.txt";
	string test_content = "Async write test";
	
	os::file f{test_file, os::O_CREATE | os::O_WR | os::O_BIN};

	auto result = test_async_write_impl(f, std::span<const char>(test_content.c_str(), test_content.size())).wait().result();
	
	CHECK(result == test_content.size());
	
	f.close();

	// Verify the written content
	{
		auto file_content = os::read_text(test_file);
		CHECK(file_content == test_content);
	}
	
	// Clean up
	std::remove(test_file.c_str());
}

TEST_CASE("file async read/write sequence") {
	string test_file = "test_async_sequence.txt";
	string expected = "First write - Second write";
	string data1 = expected.substr(0, 11); // "First write"
	string data2 = expected.substr(11);    // " - Second write"
	
	os::file f{test_file, os::O_CREATE | os::O_WR | os::O_BIN};
	
	auto result1 = test_async_write_impl(f, std::span<const char>(data1.c_str(), data1.size())).wait().result();
	CHECK(result1 == data1.size());
	
	// Debug: check file position after first write
	auto file_size1 = f.size();
	CHECK(file_size1 == data1.size());

	auto result2 = test_async_write_impl(f, std::span<const char>(data2.c_str(), data2.size())).wait().result();
	CHECK(result2 == data2.size());
	
	// Debug: check file size after second write
	auto file_size2 = f.size();
	CHECK(file_size2 == data1.size()+data2.size());
	
	f.close();

	// Read back and verify
	{
		auto file_content = os::read_text(test_file);
		CHECK(expected == file_content);
	}
	
	// Clean up
	std::remove(test_file.c_str());
}
