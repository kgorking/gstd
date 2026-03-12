#include "doctest.h"
import std;
import gs;

// Simple CPU-heavy task
static task<double> cpu_heavy_task(int iterations) {
	double result = 0;
	for (int i = 0; i < iterations; ++i) {
		result += i * std::sqrt(i);
	}
	co_return result;
}

// Task that returns void
static task<void> cpu_heavy_void_task(int iterations) {
	volatile double result = 0;
	for (int i = 0; i < iterations; ++i) {
		result += i * std::sqrt(i);
	}
	co_return;
}

TEST_CASE("task cpu-heavy computation") {
	auto t = cpu_heavy_task(1000000);
	auto result = t.wait().result();
	REQUIRE(result.has_value());
	CHECK(result.value() > 0);
}

TEST_CASE("task void return") {
	auto t = cpu_heavy_void_task(1000000);
	t.wait();
	CHECK(t.done());
}

TEST_CASE("task with co_await") {
	// Define a coroutine that awaits a task
	auto awaiter_coro = [](task<double> t) -> co<double> {
		double result = co_await t;
		co_return result / 2;
	};

	auto t = cpu_heavy_task(100000);
	auto awaiter = awaiter_coro(std::move(t));
	auto result = awaiter.wait().result();
	
	REQUIRE(result.has_value());
	CHECK(result.value() > 0);
}

TEST_CASE("task multiple parallel computations") {
	// Helper coroutine to await tasks
	auto parallel_compute = []() -> co<double> {
		auto t1 = cpu_heavy_task(500000);
		auto t2 = cpu_heavy_task(500000);
		auto t3 = cpu_heavy_task(500000);
		
		double r1 = co_await t1;
		double r2 = co_await t2;
		double r3 = co_await t3;
		
		co_return r1 + r2 + r3;
	};

	auto result = parallel_compute().wait().result();
	REQUIRE(result.has_value());
	CHECK(result.value() > 0);
}

TEST_CASE("task wait_all") {
	auto t1 = cpu_heavy_task(100000);
	auto t2 = cpu_heavy_task(100000);
	auto t3 = cpu_heavy_task(100000);
	
	// Wait for all tasks at once and get results as a tuple
	auto [r1, r2, r3] = wait_all(t1, t2, t3);
	
	REQUIRE(r1.has_value());
	REQUIRE(r2.has_value());
	REQUIRE(r3.has_value());
	CHECK(r1.value() > 0);
	CHECK(r2.value() > 0);
	CHECK(r3.value() > 0);
}

/*TEST_CASE("task wait_all with vector") {
	
	// Wait for all tasks at once and get results as a vector
	auto results = wait_all(std::vector{
        cpu_heavy_task(100000),
        cpu_heavy_task(100000),
        cpu_heavy_task(100000)
    });
	
	REQUIRE(results.size() == 3);
	CHECK(results[0] > 0);
	CHECK(results[1] > 0);
	CHECK(results[2] > 0);
}*/
