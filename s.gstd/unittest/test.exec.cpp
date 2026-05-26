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

	// Read output from the command
	auto read_result = cmd.get_stdout().read_line();
	REQUIRE(read_result == "hello");

	// Wait for the process to complete
	auto exit_code = cmd.wait();
	CHECK(exit_code == 0);
}

TEST_CASE("test.exec.read_multiple_lines") {
	#ifdef _WIN32
		auto cmd = os::exec("cmd /c echo line1&& echo line2&& echo line3");
	#else
		auto cmd = os::exec("printf 'line1\\nline2\\nline3\\n'");
	#endif
	
	CHECK(cmd);

	auto& out = cmd.get_stdout();
	REQUIRE(out.read_line() == "line1");
	REQUIRE(out.read_line() == "line2");
	REQUIRE(out.read_line() == "line3");

	auto exit_code = cmd.wait();
	CHECK(exit_code == 0);
}

TEST_CASE("test.exec.nonzero_exit_code") {
	#ifdef _WIN32
		auto cmd = os::exec("cmd /c exit 42");
	#else
		auto cmd = os::exec("exit 42");
	#endif
	
	CHECK(cmd);

	auto exit_code = cmd.wait();
	CHECK(exit_code == 42);
}

TEST_CASE("test.exec.reader_concept") {
	#ifdef _WIN32
		auto cmd = os::exec("cmd /c echo test");
	#else
		auto cmd = os::exec("echo test");
	#endif
	CHECK(cmd);

	string stdout = cmd.get_stdout().read_line();
	CHECK("test" == stdout);

	cmd.wait();
}
