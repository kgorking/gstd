#include "doctest.h"
import std;
import gs;

co<> test_void() {
	co_await std::suspend_always{};
}

co<int> test() {
	co_await test_void();
    co_return 50;
}

TEST_CASE("test.co") {
    auto result = test().result();
    CHECK(result == 50);
}
