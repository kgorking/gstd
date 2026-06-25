module;
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
export module gs:pipes_impl;
import std;
import :file;
import :string;
import :get_last_error;
import :types;

export namespace io {
	struct rw_pipes {
		file reader;
		file writer;

		rw_pipes(file r, file w) : reader(std::move(r)), writer(std::move(w)) {}
	};

	rw_pipes pipes() {
		int pipefd[2];

		if (::pipe(pipefd) < 0) {
			throw std::system_error(std::make_error_code(std::errc::io_error), get_last_error());
		}

		return { file(pipefd[0]), file(pipefd[1]) };
	}
}
