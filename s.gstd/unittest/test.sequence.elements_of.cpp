import std;
import gs;
import gs.testing;

sequence<int> gen_with_elements_of() {
    std::vector<int> v = {1, 2, 3};
    co_yield std::ranges::elements_of(v);
    std::array<int, 2> a = {4, 5};
    co_yield std::ranges::elements_of(a);
}

test sequence_elements_of = [] {
    auto gen = gen_with_elements_of();

    std::vector<int> results;
    for (int val : gen) {
        results.push_back(val);
    }

    test::assert_eq(results.size(), 5UZ, "results should have 5 elements");
    test::assert_eq(results[0], 1, "first element should be 1");
    test::assert_eq(results[1], 2, "second element should be 2");
    test::assert_eq(results[2], 3, "third element should be 3");
    test::assert_eq(results[3], 4, "fourth element should be 4");
    test::assert_eq(results[4], 5, "fifth element should be 5");
};
