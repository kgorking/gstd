export module gs.testing;

import std;
import :registry;

export class test {
public:
	// Non-copyable, non-movable for simplicity
	test(const test&) = delete;
	test(test&&) = delete;
	test& operator=(const test&) = delete;
	test& operator=(test&&) = delete;

	test(std::invocable auto&& fn, std::source_location loc = std::source_location::current()) {
		gs::testing::test_registry::register_test(std::forward<decltype(fn)>(fn), loc);
	}

	template<auto Expected, typename ActualType>
	static void equals(ActualType&& actual, const char* message = nullptr, std::source_location loc = std::source_location::current()) {
		bool cmp = false;
		if constexpr (std::integral<std::decay_t<ActualType>> && std::integral<std::decay_t<decltype(Expected)>>) {
			cmp = std::cmp_equal(actual, Expected);
		}
		else {
			cmp = (actual == Expected);
		}

		if (!cmp) {
			std::string failure_msg = std::format("{} != {} on line {}", actual, Expected, loc.line());
			if (message && *message) {
				failure_msg += " - ";
				failure_msg += message;
			}
			gs::testing::record_assertion_failure(failure_msg);
		}
	}

	template<typename ActualType, typename ExpectedType>
		requires requires(ActualType t, ExpectedType u) { t == u; }
	static void equals(ActualType const actual, ExpectedType const expected, const char* message = nullptr, std::source_location loc = std::source_location::current()) {
		bool cmp = false;
		if constexpr (std::integral<std::decay_t<ActualType>> && std::integral<std::decay_t<ExpectedType>>) {
			cmp = std::cmp_equal(actual, expected);
		}
		else {
			cmp = (actual == expected);
		}

		if (!cmp) {
			std::string failure_msg = std::format("{} != {} on line {}", actual, expected, loc.line());
			if (message && *message) {
				failure_msg += " - ";
				failure_msg += message;
			}
			gs::testing::record_assertion_failure(failure_msg);
		}
	}

	static void is_true(bool condition, std::string_view message = "", std::source_location loc = std::source_location::current()) {
		if (!condition) {
			std::string failure_msg;
			if (!message.empty()) {
				failure_msg = std::format("Assertion failed on line {}: {}", loc.line(), message);
			} else {
				failure_msg = std::format("Assertion failed on line {}", loc.line());
			}
			gs::testing::record_assertion_failure(failure_msg);
		}
	}
};

export namespace gs::testing {
	void run_all_tests() {
		test_registry::run_all_tests();
	}
}
