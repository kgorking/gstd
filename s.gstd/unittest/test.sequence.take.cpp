#include "doctest.h"

import std;
import gs;

sequence<int> numbers(int count) {
    for (int i = 0; i < count; ++i) {
        co_yield i;
    }
}

TEST_CASE("test.sequence.take") {
    std::vector<int> const expected{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    for (auto it = expected.begin(); int i : numbers(100).take(10)) {
        CHECK(*it == i);
        ++it;
    }
}
