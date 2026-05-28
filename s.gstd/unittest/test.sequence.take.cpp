#include <vector>

import std;
import gs;
import gs.testing;

sequence<int> numbers(int count) {
    for (int i = 0; i < count; ++i) {
        co_yield i;
    }
}

test sequence_take = [] {
    std::vector<int> const expected{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    for (auto it = expected.begin(); int i : numbers(100).take(10)) {
        test::equals(*it, i, "taken sequence should match expected");
        ++it;
    }
};
