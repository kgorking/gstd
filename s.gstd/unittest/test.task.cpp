#include "doctest.h"
import std;
import gs;

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

TEST_CASE("task cpu-heavy computation") {
	auto t = cpu_heavy_task(1000000);
	auto result = t.result();
	CHECK(result > 0);
}

TEST_CASE("task void return") {
	auto t = cpu_heavy_void_task(1000000);
	t.wait();
	CHECK(t.done());
}

TEST_CASE("task with co_await") {
	// Define a coroutine that awaits a task
	auto awaiter_coro = [](task<int> t) -> co<int> {
		int result = co_await t;
		co_return result;
	};

	auto t = cpu_heavy_task(100000);
	auto awaiter = awaiter_coro(std::move(t));
	int result = awaiter.result();

	CHECK(result > 0);
}

TEST_CASE("task multiple parallel computations") {
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
	CHECK(result > 0);
}

TEST_CASE("task wait_all with vector") {
	auto t1 = cpu_heavy_task(100000);
	auto t2 = cpu_heavy_task(100000);
	auto t3 = cpu_heavy_task(100000);

	auto [r1, r2, r3] = wait_all(t1, t2, t3);
	CHECK(r1 > 0);
	CHECK(r2 > 0);
	CHECK(r3 > 0);
}

TEST_CASE("task wait_all with sleepy tasks") {
	auto t1 = cpu_sleep_task(500);
	auto t2 = cpu_sleep_task(400);
	auto t3 = cpu_sleep_task(300);
	auto t4 = cpu_sleep_task(200);
	auto t5 = cpu_sleep_task(100);

	std::array<task<int>, 5> tasks{std::move(t1), std::move(t2), std::move(t3), std::move(t4), std::move(t5)};

	int last_ms = 0;
	for (int ms : wait_each(tasks)) {
    	CHECK(ms > last_ms);
		last_ms = ms;
	}
}

TEST_CASE("task channel unbuffered") {
	channel<int> ch;

	auto parallel_compute = [&ch]() -> task<void> {
		for (int i=1; i<=3; ++i) {
			ch << i;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		co_return;
	};

	auto t = parallel_compute();

	for (int i=1; i<=3; ++i) {
		int const v = ch.get();
		REQUIRE(v == i);
	}

	t.wait();
}

TEST_CASE("task channel buffered") {
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
		REQUIRE(v == i);
	}

	t.wait();
}
