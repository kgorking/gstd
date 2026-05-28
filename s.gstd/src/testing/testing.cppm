// Coroutine-based testing framework.
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

	template<typename T, typename U>
		requires requires(T t, U u) { t == u; }
	static void assert_eq(T&& actual, U&& expected, std::string_view message = "", std::source_location loc = std::source_location::current()) {
		bool cmp = false;
		if constexpr (std::integral<std::decay_t<T>> && std::integral<std::decay_t<U>>) {
			cmp = std::cmp_equal(actual, expected);
		}
		else {
			cmp = (actual == expected);
		}

		if (!cmp) {
			std::string failure_msg = std::format("{} != {} on line {}", actual, expected, loc.line());
			if (!message.empty()) {
				failure_msg += " - ";
				failure_msg += message;
			}
			gs::testing::record_assertion_failure(failure_msg);
		}
	}

	static void assert(bool condition, std::string_view message = "",
					std::source_location loc = std::source_location::current()) {
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
