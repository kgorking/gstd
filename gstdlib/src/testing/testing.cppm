export module gs:testing;

import std;
import :registry;

export class test {
public:
	// Non-copyable, non-movable
	test(const test&) = delete;
	test(test&&) = delete;
	test& operator=(const test&) = delete;
	test& operator=(test&&) = delete;

	constexpr test(std::invocable auto&& fn, std::source_location loc = std::source_location::current()) {
		if consteval {
			fn();
		}
		else {
			gs::testing::test_registry::register_test(std::forward<decltype(fn)>(fn), loc);
		}
	}

	template<typename T = bool>
	constexpr static void is_true(bool condition, const char* message = nullptr, std::source_location loc = std::source_location::current()) {
		if consteval {
			if (!condition) {
				throw;
			}
		}
		else {
			if (!condition) {
				register_failure("is_true", message, loc);
			}
		}
	}

	template<typename ActualType, typename ExpectedType>
		requires requires(ActualType t, ExpectedType u) { t == u; }
	constexpr static void equals(ActualType const& actual, ExpectedType const& expected, const char* message = nullptr, std::source_location loc = std::source_location::current()) {
		bool cmp = false;
		if constexpr (std::signed_integral<ActualType> != std::signed_integral<ExpectedType>) {
			if constexpr (std::signed_integral<ActualType>) {
				cmp = (actual == static_cast<std::make_signed_t<ExpectedType>>(expected));
			}
			else {
				cmp = (static_cast<std::make_signed_t<ActualType>>(actual) == expected);
			}
		}
		else {
			cmp = (actual == expected);
		}

		if (!cmp) {
			std::string failure_msg = std::format("{} != {}", actual, expected);
			if (message && *message) {
				failure_msg += " - ";
				failure_msg += message;
			}
			register_failure("equals", failure_msg.c_str(), loc);
		}
	}

private:
	constexpr static void register_failure(const char* op, const char* message, std::source_location loc) noexcept {
		if (message) {
			std::string failure_msg = std::format("{} failed on line {}: {}", op, loc.line(), message);
			gs::testing::record_assertion_failure(failure_msg);
		}
		else {
			std::string failure_msg = std::format("{} failed on line {}", op, loc.line());
			gs::testing::record_assertion_failure(failure_msg);
		}
	}
};

export namespace gs::testing {
	void run_all_tests() {
		test_registry::run_all_tests();
	}
}
