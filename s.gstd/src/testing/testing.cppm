// Coroutine-based testing framework.
export module gs.testing;

import std;
import :registry;

export class test {
public:
	struct promise_type {
		std::string test_name;
		std::function<void()> test_fn;

		promise_type() = default;

		test get_return_object() {
			// Capture the current coroutine handle and create a callable
			auto h = std::coroutine_handle<promise_type>::from_promise(*this);

			// Register the test function
			gs::testing::test_registry::register_test(
				test_name.empty() ? "unnamed_test" : test_name,
				[h]() {
					// Resume the coroutine to execute the test body
					if (!h.done()) {
						h.resume();
					}
				}
			);

			return test{h};
		}

		std::suspend_never initial_suspend() { return {}; }
		std::suspend_always final_suspend() noexcept { return {}; }

		void unhandled_exception() {
			// Let the test registry handle exceptions
		}

		void return_void() {}
	};

	std::coroutine_handle<promise_type> handle;

	test() = default;

	test(std::coroutine_handle<promise_type> h) : handle(h) {}

	~test() {
		if (handle) {
			handle.destroy();
		}
	}

	// Non-copyable, non-movable for simplicity
	test(const test&) = delete;
	test& operator=(const test&) = delete;

	// Static assertion helper
	template<typename Expr>
	static void assert_true(Expr&& expr, std::string_view message = "", 
							std::source_location loc = std::source_location::current()) {
		if (!static_cast<bool>(expr)) {
			std::string failure_msg;
			if (!message.empty()) {
				failure_msg = std::format("Assertion failed at {}:{}: {}",
					loc.file_name(), loc.line(), message);
			} else {
				failure_msg = std::format("Assertion failed at {}:{}",
					loc.file_name(), loc.line());
			}
			gs::testing::record_assertion_failure(failure_msg);
		}
	}

	// Templated assert for flexible usage
	template<typename T, typename U>
		requires requires(T t, U u) { t == u; }
	static void assert_eq(T&& actual, U&& expected, std::string_view message = "",
							std::source_location loc = std::source_location::current()) {
		if (!(actual == expected)) {
			std::string failure_msg;
			if constexpr (std::is_integral_v<T> && std::is_integral_v<U>) {
				failure_msg = std::format("Expected {}, got {} at {}:{}",
					expected, actual, loc.file_name(), loc.line());
			} else {
				failure_msg = std::format("Equality assertion failed at {}:{}",
					loc.file_name(), loc.line());
			}
			if (!message.empty()) {
				failure_msg += " - ";
				failure_msg += message;
			}
			gs::testing::record_assertion_failure(failure_msg);
		}
	}

	// Convenience assert for boolean expressions
	static void assert(bool condition, std::string_view message = "",
					std::source_location loc = std::source_location::current()) {
		if (!condition) {
			std::string failure_msg;
			if (!message.empty()) {
				failure_msg = std::format("Assertion failed at {}:{}: {}",
					loc.file_name(), loc.line(), message);
			} else {
				failure_msg = std::format("Assertion failed at {}:{}",
					loc.file_name(), loc.line());
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
