import std;
import gs;
import gs.testing;

std::atomic_int test_counter = 0;

// Simple CPU-heavy task
static task<int> cpu_heavy_task(int iterations) {
	int result = 100 + std::rand()%1024;
	std::this_thread::sleep_for(std::chrono::milliseconds(result));
	co_return result;
}

// Task that returns void
static task<void> cpu_heavy_void_task(int iterations) {
	int result = 100 + std::rand()%400;
	std::this_thread::sleep_for(std::chrono::milliseconds(result));
	co_return;
}

// Task that sleeps for a specified duration
static task<int> cpu_sleep_task(int milliseconds) {
	std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
	co_return milliseconds;

}

static task<int> nested_tasks_1() { co_return co_await cpu_sleep_task(5) + co_await cpu_sleep_task(5); }
static task<int> nested_tasks_2() { co_return co_await nested_tasks_1(); }
static task<int> nested_tasks_3() { co_return co_await nested_tasks_2(); }
static task<int> nested_tasks_4() { co_return co_await nested_tasks_3(); }
static task<int> nested_tasks_5() { co_return co_await nested_tasks_4(); }

test task_many_tasks = [] {
	auto t = nested_tasks_5();
	auto result = t.result();
	test::equals(result, 10, "nested tasks should return 10");
};

test task_cpu_heavy_computation = [] {
	auto t = cpu_heavy_task(1000000);
	auto result = t.result();
	test::is_true(result > 0, "result should be positive");
};

test task_void_return = [] {
	auto t = cpu_heavy_void_task(1000000);
	t.wait();
	test::is_true(t.done(), "task should be done");
};

test task_with_co_await = [] {
	// Define a coroutine that awaits a task
	auto awaiter_coro = [](task<int> t) -> co<int> {
		int result = co_await t;
		co_return result;
	};

	try {
		auto t = cpu_heavy_task(1000000);
		auto awaiter = awaiter_coro(std::move(t));
		int result = awaiter.result();
		test::is_true(result > 0, "awaited result should be positive");
	} catch (const std::exception& ex) {
		std::println("Exception in test: {}", ex.what());
		throw;
	}
};

test task_multiple_parallel_computations = [] {
	// Helper coroutine to await tasks
	auto parallel_compute = []() -> task<int> {
		auto t1 = cpu_heavy_task(500000);
		auto t2 = cpu_heavy_task(500000);
		auto t3 = cpu_heavy_task(500000);

		int r1 = co_await t1;
		int r2 = co_await t2;
		int r3 = co_await t3;

		co_return r1 + r2 + r3;
	};

	auto result = parallel_compute().result();
	test::is_true(result > 0, "parallel computation result should be positive");
};

test task_wait_all_with_vector = [] {
	auto t1 = cpu_heavy_task(100000);
	auto t2 = cpu_heavy_task(100000);
	auto t3 = cpu_heavy_task(100000);

	auto [r1, r2, r3] = wait_all(t1, t2, t3);
	test::is_true(r1 > 0, "r1 should be positive");
	test::is_true(r2 > 0, "r2 should be positive");
	test::is_true(r3 > 0, "r3 should be positive");
};

test task_channel_unbuffered = [] {
	channel<int> ch;

	auto message_sender = [&ch]() -> task<void> {
		for (int i=1; i<=3; ++i) {
			ch << i;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		co_return;
	};

	auto t = message_sender();

	for (int i=1; i<=3; ++i) {
		int const v = ch.get();
		test::equals(v, i, "channel value should match");
	}
};

test task_channel_buffered = [] {
	channel<int, 3> ch;

	// Helper coroutine to await tasks
	auto message_sender = [&ch]() -> task<void> {
		for (int i=1; i<=3; ++i) {
			ch << i;
		}
		co_return;
	};

	auto t = message_sender();

	for (int i=1; i<=3; ++i) {
		int const v = ch.get();
		test::equals(v, i, "channel value should match");
	}
};
