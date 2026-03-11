export module gs:pipes_impl;
import std;
import :file;
import :string;

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

export namespace os {
	struct rw_pipes {
		file reader;
		file writer;

		rw_pipes(file r, file w) : reader(std::move(r)), writer(std::move(w)) {}
	};

	rw_pipes pipes() {
		int pipefd[2];

		if (::pipe(pipefd) < 0) {
			return { file(-1), file(-1) };
		}

		return { file(pipefd[0]), file(pipefd[1]) };
	}
}
