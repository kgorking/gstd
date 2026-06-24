module;
#include <windows.h>
export module gs:pipes_impl;
import std;
import :file;
import :LineWriter;
import :string;
import :get_last_error;

export namespace os {
	struct rw_pipes {
		file reader;
		file writer;

		rw_pipes(file r, file w) : reader(std::move(r)), writer(std::move(w)) {}
	};

	rw_pipes pipes() {
		HANDLE read_handle, write_handle;
		SECURITY_ATTRIBUTES sa{ sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };

		if (!CreatePipe(&read_handle, &write_handle, &sa, 0)) {
			throw std::system_error(std::make_error_code(std::errc::io_error), get_last_std_error());
		}

		return { file(read_handle), file(write_handle) };
	}
}
