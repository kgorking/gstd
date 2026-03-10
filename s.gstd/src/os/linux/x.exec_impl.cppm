#if !defined(_WIN32) && !defined(_WIN64)

export module gs:exec_impl;
import std;
import :file;
import :string;

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>

export namespace os {
	class command {
		pid_t child_pid;
		file stdout_pipe;
		file stdin_pipe;

	public:
		command(pid_t pid, file out, file in) 
			: child_pid(pid), stdout_pipe(std::move(out)), stdin_pipe(std::move(in)) {}

		command(const command&) = delete;
		command& operator=(const command&) = delete;

		command(command&& other) noexcept 
			: child_pid(other.child_pid), 
			  stdout_pipe(std::move(other.stdout_pipe)), 
			  stdin_pipe(std::move(other.stdin_pipe)) {
			other.child_pid = -1;
		}

		command& operator=(command&& other) noexcept {
			if (this != &other) {
				close();
				child_pid = other.child_pid;
				stdout_pipe = std::move(other.stdout_pipe);
				stdin_pipe = std::move(other.stdin_pipe);
				other.child_pid = -1;
			}
			return *this;
		}

		~command() {
			close();
		}

		void close() {
			if (child_pid > 0) {
				// Don't force close - let the process finish
				child_pid = -1;
			}
			stdout_pipe.close();
			stdin_pipe.close();
		}

		operator bool() const {
			return child_pid > 0;
		}

		// Wait for the process to complete and return the exit code
		std::expected<int, std::error_code> wait() {
			if (child_pid <= 0) {
				return std::unexpected(std::make_error_code(std::errc::bad_file_descriptor));
			}

			int status = 0;
			pid_t wait_result = ::waitpid(child_pid, &status, 0);

			if (wait_result < 0) {
				return std::unexpected(std::make_error_code(std::errc::io_error));
			}

			int exit_code = 0;
			if (WIFEXITED(status)) {
				exit_code = WEXITSTATUS(status);
			} else if (WIFSIGNALED(status)) {
				exit_code = -WTERMSIG(status);
			}

			return exit_code;
		}

		// Get the stdout pipe for reading output
		file& get_stdout() {
			return stdout_pipe;
		}

		// Get the stdin pipe for writing input
		file& get_stdin() {
			return stdin_pipe;
		}
	};

	command exec(string cmd) {
		int stdout_pipe[2];
		int stdin_pipe[2];

		// Create pipes
		if (::pipe(stdout_pipe) < 0) {
			return { -1, file(-1), file(-1) };
		}

		if (::pipe(stdin_pipe) < 0) {
			::close(stdout_pipe[0]);
			::close(stdout_pipe[1]);
			return { -1, file(-1), file(-1) };
		}

		pid_t child_pid = ::fork();

		if (child_pid < 0) {
			// Fork failed
			::close(stdout_pipe[0]);
			::close(stdout_pipe[1]);
			::close(stdin_pipe[0]);
			::close(stdin_pipe[1]);
			return { -1, file(-1), file(-1) };
		}

		if (child_pid == 0) {
			// Child process
			// Redirect stdout to pipe write end
			if (dup2(stdout_pipe[1], STDOUT_FILENO) < 0) {
				::exit(1);
			}

			// Redirect stderr to pipe write end
			if (dup2(stdout_pipe[1], STDERR_FILENO) < 0) {
				::exit(1);
			}

			// Redirect stdin from pipe read end
			if (dup2(stdin_pipe[0], STDIN_FILENO) < 0) {
				::exit(1);
			}

			// Close all pipe file descriptors in child
			::close(stdout_pipe[0]);
			::close(stdout_pipe[1]);
			::close(stdin_pipe[0]);
			::close(stdin_pipe[1]);

			// Execute the command via shell
			::execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);

			// If execl returns, an error occurred
			::exit(127);
		}

		// Parent process
		// Close the child side of pipes
		::close(stdout_pipe[1]);
		::close(stdin_pipe[0]);

		return { child_pid, file(stdout_pipe[0]), file(stdin_pipe[1]) };
	}
}

export namespace os {
	using os::exec;
};

#endif // !defined(_WIN32) && !defined(_WIN64)
