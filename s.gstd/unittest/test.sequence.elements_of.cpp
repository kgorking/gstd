import std;
import gs;
#include "doctest.h"

sequence<int> gen_with_elements_of() {
    std::vector<int> v = {1, 2, 3};
    co_yield std::ranges::elements_of(v);
    std::array<int, 2> a = {4, 5};
    co_yield std::ranges::elements_of(a);
}

TEST_CASE("test.sequence.elements_of") {
    auto gen = gen_with_elements_of();
    
    std::vector<int> results;
    for (int val : gen) {
        results.push_back(val);
    }
    
    CHECK(results.size() == 5);
    CHECK(results[0] == 1);
    CHECK(results[1] == 2);
    CHECK(results[2] == 3);
    CHECK(results[3] == 4);
    CHECK(results[4] == 5);
}
