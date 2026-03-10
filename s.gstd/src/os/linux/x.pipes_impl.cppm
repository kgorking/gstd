#if !defined(_WIN32) && !defined(_WIN64)

export module gs:pipes_impl;
import std;
import :file;
import :string;

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

export namespace os {
	struct pipes {
		file reader;
		file writer;

		pipes(file r, file w) : reader(std::move(r)), writer(std::move(w)) {}
	};

	pipes create_pipes() {
		int pipefd[2];

		if (::pipe(pipefd) < 0) {
			return { file(-1), file(-1) };
		}

		return { file(pipefd[0]), file(pipefd[1]) };
	}
}

export namespace os {
	using os::create_pipes;
	
	inline pipes pipes() {
		return create_pipes();
	}
};

#endif // !defined(_WIN32) && !defined(_WIN64)
