import gs;
import gs.testing;
import std;

test exec_basic_command = [] {
	#ifdef _WIN32
		auto cmd = os::exec("cmd /c echo hello");
	#else
		auto cmd = os::exec("echo hello");
	#endif
	test::is_true(cmd, "exec should succeed");

	// Read output from the command
	auto read_result = cmd.get_stdout().read_line();
	test::equals(read_result, "hello", "output should be 'hello'");

	// Wait for the process to complete
	auto exit_code = cmd.wait();
	test::equals(exit_code, 0, "exit code should be 0");
};

test exec_read_multiple_lines = [] {
	#ifdef _WIN32
		auto cmd = os::exec("cmd /c echo line1&& echo line2&& echo line3");
	#else
		auto cmd = os::exec("printf 'line1\\nline2\\nline3\\n'");
	#endif

	test::is_true(cmd, "exec should succeed");

	auto& out = cmd.get_stdout();
	test::equals(out.read_line(), "line1", "first line should match");
	test::equals(out.read_line(), "line2", "second line should match");
	test::equals(out.read_line(), "line3", "third line should match");

	auto exit_code = cmd.wait();
	test::equals(exit_code, 0, "exit code should be 0");
};

test exec_nonzero_exit_code = [] {
	#ifdef _WIN32
		auto cmd = os::exec("cmd /c exit 42");
	#else
		auto cmd = os::exec("exit 42");
	#endif

	test::is_true(cmd, "exec should succeed");

	auto exit_code = cmd.wait();
	test::equals(exit_code, 42, "exit code should be 42");
};

test exec_reader_concept = [] {
	#ifdef _WIN32
		auto cmd = os::exec("cmd /c echo test");
	#else
		auto cmd = os::exec("echo test");
	#endif
	test::is_true(cmd, "exec should succeed");

	string stdout = cmd.get_stdout().read_line();
	test::equals(stdout, "test", "output should match");

	cmd.wait();
};
