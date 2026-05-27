import std;
import gs;
import gs.testing;

co<> test_void() {
	co_await std::suspend_always{};
}

co<int> simple_test() {
	co_await test_void();
	co_return 50;
}

test test_co_result = [] {
	auto result = simple_test().result();
	test::assert_eq(result, 50, "coroutine result should be 50");
};
