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
	static void assert_eq(T&& actual, U&& expected, std::string_view message = "",
							std::source_location loc = std::source_location::current()) {
		if (!(actual == expected)) {
			std::string failure_msg;
			if constexpr (std::is_integral_v<T> && std::is_integral_v<U>) {
				failure_msg = std::format("Expected {}, got {} on line {}", expected, actual, loc.line());
			} else {
				failure_msg = std::format("Equality assertion failed on line {}", loc.line());
			}
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
