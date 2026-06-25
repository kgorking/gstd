module;
#include <windows.h>
export module gs:exec_impl;
import std;
import :file;
import :string;
import :pipes;
import :get_last_error;

export namespace os {
	class command {
		HANDLE process_handle;
		io::file stdout_pipe;
		io::file stdin_pipe;

	public:
		command(HANDLE handle, io::file out, io::file in) 
			: process_handle(handle), stdout_pipe(std::move(out)), stdin_pipe(std::move(in)) {}

		command(const command&) = delete;
		command& operator=(const command&) = delete;

		command(command&& other) noexcept 
			: process_handle(std::exchange(other.process_handle, INVALID_HANDLE_VALUE)),
			  stdout_pipe(std::move(other.stdout_pipe)), 
			  stdin_pipe(std::move(other.stdin_pipe)) {}

		command& operator=(command&& other) noexcept {
			if (this != &other) {
				close();
				process_handle = std::exchange(other.process_handle, INVALID_HANDLE_VALUE);
				stdout_pipe = std::move(other.stdout_pipe);
				stdin_pipe = std::move(other.stdin_pipe);
			}
			return *this;
		}

		~command() {
			close();
		}

		void close() {
			if (process_handle != INVALID_HANDLE_VALUE) {
				CloseHandle(process_handle);
				process_handle = INVALID_HANDLE_VALUE;
			}
			stdout_pipe.close();
			stdin_pipe.close();
		}

		operator bool() const {
			return process_handle != INVALID_HANDLE_VALUE;
		}

		// Wait for the process to complete and return the exit code
		int wait() {
			if (process_handle == INVALID_HANDLE_VALUE)
				throw std::system_error(std::make_error_code(std::errc::bad_file_descriptor));

			DWORD wait_result = WaitForSingleObject(process_handle, INFINITE);
			if (wait_result != WAIT_OBJECT_0)
				throw std::system_error(std::make_error_code(std::errc::io_error), get_last_std_error());

			DWORD exit_code = 0;
			if (!GetExitCodeProcess(process_handle, &exit_code))
				throw std::system_error(std::make_error_code(std::errc::io_error), get_last_std_error());

			return static_cast<int>(exit_code);
		}

		io::file& get_stdout() { return stdout_pipe; }
		io::file& get_stdin()  { return stdin_pipe;  }
	};

	command exec(string cmd) {
		// Make a mutable copy of the command string for CreateProcessA
		std::string cmd_str(cmd.c_str());

		// Create pipes for stdout (child writes, parent reads) and stdin (parent writes, child reads)
		auto stdout = io::pipes();
		auto stdin = io::pipes();
		SetHandleInformation(stdout.reader.get_os_handle(), HANDLE_FLAG_INHERIT, 0);
		SetHandleInformation(stdin.writer.get_os_handle(), HANDLE_FLAG_INHERIT, 0);

		// Set up process info
		STARTUPINFOA startup_info = {};
		startup_info.cb = sizeof(startup_info);
		startup_info.hStdOutput = stdout.writer.get_os_handle();   // Child writes to this
		startup_info.hStdError = stdout.writer.get_os_handle();    // Child writes to this
		startup_info.hStdInput = stdin.reader.get_os_handle();     // Child reads from this
		startup_info.dwFlags = STARTF_USESTDHANDLES;

		// Create the process
		PROCESS_INFORMATION process_info{};
		if (!CreateProcessA(nullptr, cmd_str.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &startup_info, &process_info)) {
			throw std::system_error(std::make_error_code(std::errc::io_error), get_last_std_error());
		}

		CloseHandle(process_info.hThread);

		return { process_info.hProcess, std::move(stdout.reader), std::move(stdin.writer) };
	}
}

export namespace os {
	using os::exec;
};
