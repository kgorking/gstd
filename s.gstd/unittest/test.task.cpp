import std;
import gs;
import gs.testing;

static task<int> cpu_heavy_task(int iterations) {
	int result = 100 + std::rand() % 1024;
	std::this_thread::sleep_for(std::chrono::milliseconds(result));
	co_return 1;
}

static task<void> cpu_heavy_void_task(int iterations) {
	int result = 100 + std::rand() % 400;
	std::this_thread::sleep_for(std::chrono::milliseconds(result));
	co_return;
}

static task<int> cpu_sleep_task() {
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	co_return 5;
}

test task_in_task = [] {
	auto yielder = [] -> task<int> {
		co_yield 1;
		co_yield 2;
		co_yield 3;
		co_return 1122;
		};

	auto tester = [&] -> task<int> {
		auto y = yielder();
		if (1 != co_await y) co_return 0;
		if (2 != co_await y) co_return 0;
		if (3 != co_await y) co_return 0;
		if (1122 != co_await y) co_return 0;
		co_return 1;
		};

	int result = tester().result();
	test::is_true(result == 1);
	};

test task_multiple_parallel_computations = [] {
	auto parallel_compute = []() -> task<int> {
		auto t1 = cpu_heavy_task(500);
		auto t2 = cpu_heavy_task(500);
		auto t3 = cpu_heavy_task(500);

		{
			int r3 = co_await t3;
			int r2 = co_await t2;
			int r1 = co_await t1;
			co_return r1 + r2 + r3;
		}
		};

	auto result = parallel_compute().result();
	test::is_true(result > 0, "parallel computation result should be positive");
	};

test task_void_return = [] {
	auto y = cpu_heavy_void_task(1000000);
	y.wait();
	test::is_true(y.done(), "task should be done");
	};


static task<int> nested_tasks_1() { co_return co_await cpu_sleep_task() + co_await cpu_sleep_task(); }
static task<int> nested_tasks_2() { co_return co_await nested_tasks_1(); }
static task<int> nested_tasks_3() { co_return co_await nested_tasks_2(); }
static task<int> nested_tasks_4() { co_return co_await nested_tasks_3(); }
static task<int> nested_tasks_5() { co_return co_await nested_tasks_4(); }

test task_many_tasks = [] {
	auto y = nested_tasks_5();
	auto result = y.result();
	test::equals(result, 10, "nested tasks should return 10");
	};

test task_cpu_heavy_computation = [] {
	auto y = cpu_heavy_task(1000000);
	auto result = y.result();
	test::is_true(result > 0, "result should be positive");
	};

test task_with_co_await = [] {
	auto awaiter_helper = [](task<int> y) -> task<int> {
		int result = co_await y;
		co_return result;
		};

	try {
		auto y = cpu_heavy_task(1000000);
		auto awaiter = awaiter_helper(std::move(y));
		int result = awaiter.result();
		test::is_true(result > 0, "awaited result should be positive");
	}
	catch (const std::exception& ex) {
		std::println("Exception in test: {}", ex.what());
		throw;
	}
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

test task_channel_buffered = [] {
	channel<int, 3> ch;

	auto message_sender = [&ch]() -> task<void> {
		for (int i = 1; i <= 3; ++i) {
			ch << i;
		}
		co_return;
		};

	auto y = message_sender();

	for (int i = 1; i <= 3; ++i) {
		int const v = ch.get();
		test::equals(v, i, "channel value should match");
	}
	};

test task_channel_unbuffered = [] {
	channel<int> ch;

	auto message_sender = [&ch]() -> task<void> {
		for (int i = 1; i <= 3; ++i) {
			ch << i;
		}
		co_return;
		};

	auto y = message_sender();
	for (int i = 1; i <= 3; ++i) {
		int const v = *ch;
		test::equals(v, i, "channel value should match");
	}
	};
