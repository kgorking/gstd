#include "doctest.h"
import gs;
import std;

TEST_CASE("test.exec.basic_command") {
	#ifdef _WIN32
		auto cmd = os::exec("cmd /c echo hello");
	#else
		auto cmd = os::exec("echo hello");
	#endif
	REQUIRE(cmd);

	// Close the write end of stdin since we're not using it
	// This helps the process terminate properly
	cmd.get_stdin().close();

	// Read output from the command
	char buffer[256] = {};
	auto read_result = cmd.get_stdout().read(std::span<char>(buffer, sizeof(buffer) - 1));
	if (!read_result) {
		std::println("Read failed with error: {}", read_result.error().message());
	}
	CHECK(read_result);
	if (read_result) {
		CHECK(*read_result > 0);  // Should read at least some bytes
	}

	// Wait for the process to complete
	auto exit_code = cmd.wait();
	CHECK(exit_code);
	CHECK(*exit_code == 0);
}

TEST_CASE("test.exec.read_multiple_lines") {
	#ifdef _WIN32
		auto cmd = os::exec("cmd /c echo line1 && echo line2 && echo line3");
	#else
		auto cmd = os::exec("printf 'line1\\nline2\\nline3\\n'");
	#endif
	
	CHECK(cmd);

	std::vector<char> buffer(256);
	auto read_result = cmd.get_stdout().read(std::span(buffer.data(), buffer.size()));
	CHECK(read_result);
	CHECK(*read_result > 0);

	auto exit_code = cmd.wait();
	CHECK(exit_code);
	CHECK(*exit_code == 0);
}

TEST_CASE("test.exec.nonzero_exit_code") {
	#ifdef _WIN32
		auto cmd = os::exec("cmd /c exit 42");
	#else
		auto cmd = os::exec("exit 42");
	#endif
	
	CHECK(cmd);

	auto exit_code = cmd.wait();
	CHECK(exit_code);
	CHECK(*exit_code == 42);
}

TEST_CASE("test.exec.reader_concept") {
	#ifdef _WIN32
		auto cmd = os::exec("cmd /c echo test");
	#else
		auto cmd = os::exec("echo test");
	#endif
	CHECK(cmd);

	// Verify stdout satisfies Reader concept
	static_assert(Reader<std::remove_reference_t<decltype(cmd.get_stdout())>>);

	auto exit_code = cmd.wait();
	CHECK(exit_code);
}
