module;
#include <windows.h>
export module gs:pipes_impl;
import std;
import :file;
import :LineWriter;
import :string;

export namespace os {
	struct rw_pipes {
		file reader;
		file writer;

		rw_pipes(file r, file w) : reader(std::move(r)), writer(std::move(w)) {}
	};

	rw_pipes pipes() {
		HANDLE read_handle, write_handle;

		if (!CreatePipe(&read_handle, &write_handle, nullptr, 0)) {
			return { file(INVALID_HANDLE_VALUE, true), file(INVALID_HANDLE_VALUE, true) };
		}

		// Skip IOCP registration for pipes since they only use synchronous I/O
		return { file(read_handle, true), file(write_handle, true) };
	}
}
