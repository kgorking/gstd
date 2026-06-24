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

    test::equals<5>(results.size(), "results should have 5 elements");
	test::equals<1>(results[0], "first element should be 1");
	test::equals<2>(results[1], "second element should be 2");
	test::equals<3>(results[2], "third element should be 3");
	test::equals<4>(results[3], "fourth element should be 4");
	test::equals<5>(results[4], "fifth element should be 5");
};
