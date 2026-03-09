#include "doctest.h"

import std;
import gs;

sequence<int> numbers(int count) {
    for (int i = 0; i < count; ++i) {
        co_yield i;
    }
}

TEST_CASE("test.sequence.take") {
    std::vector<int> const excpected{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::vector<int> actual;

    for (int i : numbers(100).take(10)) {
        actual.push_back(i);
    }
    CHECK(excpected == actual);
}
