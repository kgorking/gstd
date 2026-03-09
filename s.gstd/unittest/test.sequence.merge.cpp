#include "doctest.h"

import std;
import gs;

sequence<int> odds() {
    for (int i = 1; ; i += 2) {
        co_yield i;
    }
}

sequence<int> evens() {
    for (int i = 0; ; i += 2) {
        co_yield i;
    }
}

TEST_CASE("test.sequence.merge") {
    std::vector<int> const excpected{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::vector<int> actual;

    sequence<int> merged = odds().merge(evens());
    for (int i : merged) {
        actual.push_back(i);
        if (i==9) break;
    }
    CHECK(excpected == actual);
}
