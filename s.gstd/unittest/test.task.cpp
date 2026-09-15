import gs;
import std;

static_assert(std::is_copy_constructible_v<task<int>>);
static_assert(std::is_copy_assignable_v<task<int>>);


static task<void> cpu_heavy_task(channel<int>& ch, int r) {
	co_await thread_pool::switch_to_thread();
	int result = 100 + std::rand() % 1024;
	std::this_thread::sleep_for(std::chrono::milliseconds(result));
	ch << r;
}

static task<void> cpu_sleep_task(channel<int>& ch) {
	co_await thread_pool::switch_to_thread();
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	ch << 5;
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
		if (1 != co_await y) co_return -1;
		if (2 != co_await y) co_return -2;
		if (3 != co_await y) co_return -3;
		if (1122 != co_await y) co_return -1122;
		co_return 1;
		};

	int result = tester().result();
	test::equals(result, 1);
	};

test task_multiple_parallel_computations = [] {
	auto parallel_compute = []() -> task<int> {
		channel<int> ch;
		auto t1 = cpu_heavy_task(ch, 500);
		auto t2 = cpu_heavy_task(ch, 500);
		auto t3 = cpu_heavy_task(ch, 500);
		co_return ch.get() + ch.get() + ch.get();
		};

	auto result = parallel_compute().result();
	test::is_true(result == 1500, "parallel computation result should be 1500");
	};

static task<int> nested_tasks_1() { 
	channel<int> ch;
	cpu_sleep_task(ch);
	co_return ch.get();
}
static task<int> nested_tasks_2() { co_return co_await nested_tasks_1(); }
static task<int> nested_tasks_3() { co_return co_await nested_tasks_2(); }
static task<int> nested_tasks_4() { co_return co_await nested_tasks_3(); }
static task<int> nested_tasks_5() { co_return co_await nested_tasks_4(); }

test task_many_tasks = [] {
	auto y = nested_tasks_5();
	auto result = y.result();
	test::equals(result, 5);
	};


test task_wait_all_with_vector = [] {
	channel<int> ch;
	auto t1 = cpu_heavy_task(ch, 1);
	auto t2 = cpu_heavy_task(ch, 2);
	auto t3 = cpu_heavy_task(ch, 3);

	int result = ch.get() + ch.get() + ch.get();
	test::equals(result, 6);
	};

test task_channel_buffered = [] {
	channel<int, 3> ch;

	auto message_sender = [&ch]() -> task<void> {
		co_await thread_pool::switch_to_thread();
		for (int i = 1; i <= 3; ++i) {
			ch << i;
		}
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
		co_await thread_pool::switch_to_thread();
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
