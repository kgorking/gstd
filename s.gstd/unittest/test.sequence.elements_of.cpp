import std;
import gs;

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

    test::equals(results.size(), 5Z, "results should have 5 elements");
	test::equals(results[0], 1, "first element should be 1");
	test::equals(results[1], 2, "second element should be 2");
	test::equals(results[2], 3, "third element should be 3");
	test::equals(results[3], 4, "fourth element should be 4");
	test::equals(results[4], 5, "fifth element should be 5");
};
