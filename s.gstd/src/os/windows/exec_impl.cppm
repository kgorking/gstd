module;
#include <windows.h>
export module gs:exec_impl;
import std;
import :file;
import :string;

export namespace os {
	class command {
		HANDLE process_handle;
		file stdout_pipe;
		file stdin_pipe;

	public:
		command(HANDLE handle, file out, file in) 
			: process_handle(handle), stdout_pipe(std::move(out)), stdin_pipe(std::move(in)) {}

		command(const command&) = delete;
		command& operator=(const command&) = delete;

		command(command&& other) noexcept 
			: process_handle(other.process_handle), 
			  stdout_pipe(std::move(other.stdout_pipe)), 
			  stdin_pipe(std::move(other.stdin_pipe)) {
			other.process_handle = INVALID_HANDLE_VALUE;
		}

		command& operator=(command&& other) noexcept {
			if (this != &other) {
				close();
				process_handle = other.process_handle;
				stdout_pipe = std::move(other.stdout_pipe);
				stdin_pipe = std::move(other.stdin_pipe);
				other.process_handle = INVALID_HANDLE_VALUE;
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
		std::expected<int, std::error_code> wait() {
			if (process_handle == INVALID_HANDLE_VALUE) {
				return std::unexpected(std::make_error_code(std::errc::bad_file_descriptor));
			}

			DWORD wait_result = WaitForSingleObject(process_handle, INFINITE);
			if (wait_result != WAIT_OBJECT_0) {
				return std::unexpected(std::make_error_code(std::errc::io_error));
			}

			DWORD exit_code = 0;
			if (!GetExitCodeProcess(process_handle, &exit_code)) {
				return std::unexpected(std::make_error_code(std::errc::io_error));
			}

			return static_cast<int>(exit_code);
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
		HANDLE stdout_read, stdout_write;
		HANDLE stdin_read, stdin_write;

		// Set up SECURITY_ATTRIBUTES to make handles inheritable
		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.bInheritHandle = TRUE;
		sa.lpSecurityDescriptor = nullptr;

		// Create pipes for stdout (child writes, parent reads)
		if (!CreatePipe(&stdout_read, &stdout_write, &sa, 0)) {
            std::println("CreatePipe for stdout failed with error: {}", GetLastError());
			return { INVALID_HANDLE_VALUE, file(INVALID_HANDLE_VALUE), file(INVALID_HANDLE_VALUE) };
   		}
		// Make stdout_read non-inheritable (parent only)
		SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0);

		// Create pipes for stdin (parent writes, child reads)
		if (!CreatePipe(&stdin_read, &stdin_write, &sa, 0)) {
			CloseHandle(stdout_read);
			CloseHandle(stdout_write);
			return { INVALID_HANDLE_VALUE, file(INVALID_HANDLE_VALUE), file(INVALID_HANDLE_VALUE) };
		}
		// Make stdin_write non-inheritable (parent only)
		SetHandleInformation(stdin_write, HANDLE_FLAG_INHERIT, 0);

		// Set up process info
		STARTUPINFOA startup_info = {};
		startup_info.cb = sizeof(startup_info);
		startup_info.hStdOutput = stdout_write;   // Child writes to this
		startup_info.hStdError = stdout_write;    // Child writes to this
		startup_info.hStdInput = stdin_read;      // Child reads from this
		startup_info.dwFlags = STARTF_USESTDHANDLES;

		// Create the process
		PROCESS_INFORMATION process_info = {};
		
		// Make a mutable copy of the command string for CreateProcessA
		std::string cmd_str(cmd.c_str());
		
		if (!CreateProcessA(nullptr, cmd_str.data(), nullptr, nullptr, TRUE, 
		                    CREATE_NO_WINDOW, nullptr, nullptr, &startup_info, &process_info)) {
            LPVOID lpMsgBuf;
            DWORD dw = GetLastError(); 

            if (FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                dw,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &lpMsgBuf,
                0, NULL) != 0) {
                std::println("os::exec '{}' failed with error: \"{}\" ({})", cmd_str, (char*)lpMsgBuf, dw);
            }

            LocalFree(lpMsgBuf);

			CloseHandle(stdout_read);
			CloseHandle(stdout_write);
			CloseHandle(stdin_read);
			CloseHandle(stdin_write);
			return { INVALID_HANDLE_VALUE, file(INVALID_HANDLE_VALUE), file(INVALID_HANDLE_VALUE) };
		}

		std::println("os::exec '{}' process created with PID: {}", cmd_str, process_info.dwProcessId);

		// Close the inherited handles in parent process
		// Close the write end of stdout (only child writes to it)
		CloseHandle(stdout_write);
		// Close the read end of stdin (only child reads from it)
		CloseHandle(stdin_read);
		CloseHandle(process_info.hThread);

		return { process_info.hProcess, file(stdout_read), file(stdin_write) };
	}
}

export namespace os {
	using os::exec;
};
