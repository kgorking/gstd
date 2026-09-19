import gs;
import std;

static task<int> sleep_task(int ms) {
	co_await std::suspend_always{};
	// sike
	co_return ms;
}

test yield_all_test = [] {
	auto tasks = std::array{ sleep_task(1), sleep_task(2), sleep_task(3), sleep_task(4), sleep_task(5) };

	int expected = 0;
	int num_results = 0;
	for (int result : yield_all(std::move(tasks))) {
		num_results += 1;
		expected += result;
	}

	test::equals(expected, 1+2+3+4+5, "Sum of results does not match expected value");
	test::equals(num_results, tasks.size(), "Number of results does not match number of tasks");
	};
