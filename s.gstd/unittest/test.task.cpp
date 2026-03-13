#include "doctest.h"
import std;
import gs;

std::atomic_int test_counter = 0;

// Simple CPU-heavy task
static task<int> cpu_heavy_task(int iterations) {
    int const count = ++test_counter;
    sync_console_writer.write(string::fmt("{} Starting CPU-heavy task...\n", count));

    int result = 100 + std::rand()%1024;
    std::this_thread::sleep_for(std::chrono::milliseconds(result));

    sync_console_writer.write(string::fmt("{} Done with CPU-heavy task\n", count));
    co_return result;
}

// Task that returns void
static task<void> cpu_heavy_void_task(int iterations) {
    int const count = ++test_counter;
    sync_console_writer.write(string::fmt("{} Starting CPU-heavy task...\n", count));
    int result = 100 + std::rand()%400;
    std::this_thread::sleep_for(std::chrono::milliseconds(result));
    sync_console_writer.write(string::fmt("{} Done with CPU-heavy task\n", count));
	co_return;
}

// Task that sleeps for a specified duration
static task<int> cpu_sleep_task(int milliseconds) {
	int const count = ++test_counter;
	sync_console_writer.write(string::fmt("{} Starting sleep task for {} milliseconds...\n", count, milliseconds));
	std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
	sync_console_writer.write(string::fmt("{} Done with sleep task\n", count));
	co_return milliseconds;

}

TEST_CASE("task cpu-heavy computation") {
    std::println("task cpu-heavy computation");
	auto t = cpu_heavy_task(1000000);
	auto result = t.wait().result();
	REQUIRE(result.has_value());
	CHECK(result.value() > 0);
}

TEST_CASE("task void return") {
    std::println("task void return");
	auto t = cpu_heavy_void_task(1000000);
	t.wait();
	CHECK(t.done());
}

TEST_CASE("task with co_await") {
    std::println("task with co_await");
	// Define a coroutine that awaits a task
	auto awaiter_coro = [](task<int> t) -> co<int> {
		int result = co_await t;
		co_return result;
	};

	auto t = cpu_heavy_task(100000);
	auto awaiter = awaiter_coro(std::move(t));
	auto result = awaiter.wait().result();

	REQUIRE(result.has_value());
	CHECK(result.value() > 0);
}

TEST_CASE("task multiple parallel computations") {
    std::println("task multiple parallel computations");
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

	auto result = parallel_compute().wait().result();
	REQUIRE(result.has_value());
	CHECK(result.value() > 0);
}

TEST_CASE("task wait_all with vector") {
	auto t1 = cpu_heavy_task(100000);
	auto t2 = cpu_heavy_task(100000);
	auto t3 = cpu_heavy_task(100000);

	auto [r1, r2, r3] = wait_all(t1, t2, t3);
	REQUIRE(r1.has_value());
	REQUIRE(r2.has_value());
	REQUIRE(r3.has_value());
	CHECK(r1.value() > 0);
	CHECK(r2.value() > 0);
	CHECK(r3.value() > 0);
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

TEST_CASE("task channel") {
    std::println("task channel");

	channel<int> ch;

	// Helper coroutine to await tasks
	auto parallel_compute = [&ch]() -> task<void> {
	    std::println("sending 1 2 3");
		ch << 1 << 2 << 3; // Send values to the channel
		co_return;
	};

	parallel_compute().wait();

	int v1, v2, v3;
	ch >> v1 >> v2 >> v3; // Read values from the channel
	std::println("received {} {} {}", v1, v2, v3);
	REQUIRE(v1 == 1);
	REQUIRE(v2 == 2);
	REQUIRE(v3 == 3);
}
