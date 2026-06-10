import std;
import gs;
import gs.testing;

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

test sequence_merge = [] {
    std::vector<int> const expected{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::vector<int> actual;

    sequence<int> merged = odds().merge(evens());
    for (int i : merged) {
        actual.push_back(i);
        if (i==9) break;
    }
    test::equals(expected, actual, "merged sequence should match expected");
};
