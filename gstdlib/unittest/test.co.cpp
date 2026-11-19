import std;
import gs;

task<> test_void() {
	co_return;
}

task<int> simple_test() {
	co_await test_void();
	co_return 50;
}

test test_co_result = [] {
	auto t = simple_test();
	auto result = t.result();
	test::equals(result, 50, "coroutine result should be 50");
};
