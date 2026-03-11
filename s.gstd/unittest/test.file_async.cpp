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
	const char* test_file = "test_async_read.txt";
	const char* test_content = "Hello, async world!";
	
	// Write test content using synchronous write
	{
		os::file f(string(test_file), os::O_CREATE | os::O_WR | os::O_BIN);
		f.write(std::span<const char>(test_content, std::strlen(test_content)));
	}
	
	// Now test async read - keep file alive across the async operation
	os::file f{string(test_file)};
	char buf[256] = {0};
	
	auto result = test_async_read_impl(f, std::span(buf)).wait().result();
	
	REQUIRE(result.has_value());
	CHECK(result.value() == std::strlen(test_content));
	CHECK(std::string_view(buf, result.value()) == test_content);
	
	f.close();
	
	// Clean up
	std::remove(test_file);
}

TEST_CASE("file async write") {
	const char* test_file = "test_async_write.txt";
	const char* test_content = "Async write test";
	
	os::file f{string(test_file), os::O_CREATE | os::O_WR | os::O_BIN};
	
	auto result = test_async_write_impl(f, std::span<const char>(test_content, std::strlen(test_content))).wait().result();
	
	REQUIRE(result.has_value());
	CHECK(result.value() == std::strlen(test_content));
	
	f.close();

	// Verify the written content
	{
		os::file f2{string(test_file)};
		char buf[256] = {0};
		auto bytes_read = f2.read(std::span(buf));
		CHECK(std::string_view(buf, bytes_read) == test_content);
	}
	
	// Clean up
	std::remove(test_file);
}

TEST_CASE("file async read/write sequence") {
	const char* test_file = "test_async_sequence.txt";
	const char* data1 = "First write";
	const char* data2 = " - Second write";
	
	os::file f{string(test_file), os::O_CREATE | os::O_WR | os::O_BIN};
	
	auto result1 = test_async_write_impl(f, std::span<const char>(data1, std::strlen(data1))).wait().result();
	REQUIRE(result1.has_value());
	CHECK(result1.value() == std::strlen(data1));
	
	// Debug: check file position after first write
	auto file_size1 = f.size();
	CHECK(file_size1 == std::strlen(data1));

	auto result2 = test_async_write_impl(f, std::span<const char>(data2, std::strlen(data2))).wait().result();
	REQUIRE(result2.has_value());
	CHECK(result2.value() == std::strlen(data2));
	
	// Debug: check file size after second write
	auto file_size2 = f.size();
	CHECK(file_size2 == std::strlen(data1) + std::strlen(data2));
	
	f.close();

	// Read back and verify
	{
		os::file f2{string(test_file)};
		char buf[256] = {0};
		
		auto bytes_read = f2.read(std::span(buf));
		
		std::string expected = std::string(data1) + data2;
		// Debug output
		auto actual = std::string_view(buf, bytes_read);
		CHECK(actual == expected);
	}
	
	// Clean up
	std::remove(test_file);
}
