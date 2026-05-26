import gs;
import gs.testing;
import std;

auto exec_basic_command = [] -> test {
	#ifdef _WIN32
		auto cmd = os::exec("cmd /c echo hello");
	#else
		auto cmd = os::exec("echo hello");
	#endif
	test::assert(cmd, "exec should succeed");

	// Read output from the command
	auto read_result = cmd.get_stdout().read_line();
	test::assert_eq(read_result, string("hello"), "output should be 'hello'");

	// Wait for the process to complete
	auto exit_code = cmd.wait();
	test::assert_eq(exit_code, 0, "exit code should be 0");
};

auto exec_read_multiple_lines = [] -> test {
	#ifdef _WIN32
		auto cmd = os::exec("cmd /c echo line1&& echo line2&& echo line3");
	#else
		auto cmd = os::exec("printf 'line1\\nline2\\nline3\\n'");
	#endif

	test::assert(cmd, "exec should succeed");

	auto& out = cmd.get_stdout();
	test::assert_eq(out.read_line(), string("line1"), "first line should match");
	test::assert_eq(out.read_line(), string("line2"), "second line should match");
	test::assert_eq(out.read_line(), string("line3"), "third line should match");

	auto exit_code = cmd.wait();
	test::assert_eq(exit_code, 0, "exit code should be 0");
};

auto exec_nonzero_exit_code = [] -> test {
	#ifdef _WIN32
		auto cmd = os::exec("cmd /c exit 42");
	#else
		auto cmd = os::exec("exit 42");
	#endif

	test::assert(cmd, "exec should succeed");

	auto exit_code = cmd.wait();
	test::assert_eq(exit_code, 42, "exit code should be 42");
};

auto exec_reader_concept = [] -> test {
	#ifdef _WIN32
		auto cmd = os::exec("cmd /c echo test");
	#else
		auto cmd = os::exec("echo test");
	#endif
	test::assert(cmd, "exec should succeed");

	string stdout = cmd.get_stdout().read_line();
	test::assert_eq(string("test"), stdout, "output should match");

	cmd.wait();
};
