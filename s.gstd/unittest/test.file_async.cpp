import std;
import gs;
import gs.testing;

// Test coroutine for async read
static co<std::int64_t> test_async_read_impl(os::file& f, std::span<char> buf) {
	co_return co_await f.read_async(buf);
}

// Test coroutine for async write
static co<std::int64_t> test_async_write_impl(os::file& f, std::span<const char> buf) {
	co_return co_await f.write_async(buf);
}

test file_async_read = [] {
	// Create a temporary test file
	string test_file = "test_async_read.txt";
	string test_content = "Hello, async world!";

	// Write test content using synchronous write
	os::write_text(test_file, test_content);

	// Now test async read - keep file alive across the async operation
	os::file f{test_file};
	char buf[256] = {0};

	auto result = test_async_read_impl(f, std::span(buf)).result();
	buf[result] = '\0'; // Null-terminate the buffer

	test::assert_eq(result, test_content.size(), "read size should match");
	test::assert_eq(string(buf), test_content, "read content should match");

	f.close();

	// Clean up
	std::remove(test_file.c_str());
};

test file_async_write = [] {
	string test_file = "test_async_write.txt";
	string test_content = "Async write test";

	os::file f{test_file, os::O_CREATE | os::O_WR | os::O_BIN};

	auto result = test_async_write_impl(f, std::span<const char>(test_content.c_str(), test_content.size())).result();

	test::assert_eq(result, test_content.size(), "write size should match");

	f.close();

	// Verify the written content
	{
		auto file_content = os::read_text(test_file);
		test::assert_eq(file_content, test_content, "file content should match");
	}

	// Clean up
	std::remove(test_file.c_str());
};

test file_async_read_write_sequence = [] {
	string test_file = "test_async_sequence.txt";
	string expected = "First write - Second write";
	string data1 = expected.substr(0, 11); // "First write"
	string data2 = expected.substr(11);    // " - Second write"

	os::file f{test_file, os::O_CREATE | os::O_WR | os::O_BIN};

	auto result1 = test_async_write_impl(f, std::span<const char>(data1.c_str(), data1.size())).result();
	test::assert_eq(result1, data1.size(), "first write size should match");

	auto file_size1 = f.size();
	test::assert_eq(file_size1, data1.size(), "file size after first write");

	auto result2 = test_async_write_impl(f, std::span<const char>(data2.c_str(), data2.size())).result();
	test::assert_eq(result2, data2.size(), "second write size should match");

	auto file_size2 = f.size();
	test::assert_eq(file_size2, data1.size() + data2.size(), "file size after second write");

	f.close();

	// Read back and verify
	{
		auto file_content = os::read_text(test_file);
		test::assert_eq(expected, file_content, "final content should match");
	}

	// Clean up
	std::remove(test_file.c_str());
};
