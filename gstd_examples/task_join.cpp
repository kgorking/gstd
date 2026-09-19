import gs;
import std;

using namespace std::chrono_literals;

static task<int> sleep_task(int ms) {
	co_await std::suspend_always{};
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
	co_return 1;
}

int main() {
	// Set the global locale to US English with UTF-8 encoding
	std::locale::global(std::locale("da_DK.UTF-8"));
	std::cout.imbue(std::locale());

	// Initialize thread pool
	std::ignore = thread_pool::instance();
	std::this_thread::sleep_for(10ms);

	uint64 counter = 0;
	auto const start = std::chrono::system_clock::now();
	for (int i = 0; i < 100000; i++) {
		auto tasks = std::array{
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0)
		};

		for (int result : yield_all(tasks))
			counter += result;
	}

	auto const end = std::chrono::system_clock::now();
	auto const diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	auto const diff_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
	std::println("time: {:L} milliseconds for {:L} task invocations, {} ns/task", diff_ms.count(), counter, diff_ns.count() / counter);
}
