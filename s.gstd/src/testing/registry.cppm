// Test registry and runner for the coroutine-based testing framework.
export module gs.testing:registry;

import std;

export namespace gs::testing {
	struct test_result {
		std::string test_name;
		bool passed = true;
		std::vector<std::string> failures;
	};

	class test_registry {
	private:
		struct test_entry {
			std::string name;
			std::function<void()> run;
		};

		static test_registry& instance() {
			static test_registry reg;
			return reg;
		}

		std::vector<test_entry> tests;
		std::mutex registry_mutex;
		std::vector<test_result> results;

	public:
		static void register_test(std::string name, std::function<void()> test_fn) {
			auto& reg = instance();
			std::lock_guard<std::mutex> lock(reg.registry_mutex);
			reg.tests.emplace_back(test_entry{std::move(name), std::move(test_fn)});
		}

		static void run_all_tests() {
			auto& reg = instance();
			std::lock_guard<std::mutex> lock(reg.registry_mutex);

			reg.results.clear();
			int passed = 0;
			int failed = 0;

			for (const auto& entry : reg.tests) {
				test_result result;
				result.test_name = entry.name;

				try {
					// Set current test context for assertions
					current_test_context = &result;
					entry.run();
					if (result.failures.empty()) {
						result.passed = true;
						passed++;
					} else {
						result.passed = false;
						failed++;
					}
				} catch (const std::exception& e) {
					result.passed = false;
					result.failures.push_back(std::string("Exception: ") + e.what());
					failed++;
				} catch (...) {
					result.passed = false;
					result.failures.push_back("Unknown exception");
					failed++;
				}

				current_test_context = nullptr;
				reg.results.push_back(result);
			}

			// Print results
			print_results(passed, failed, reg.results);
		}

		static thread_local test_result* current_test_context;

	private:
		static void print_results(int passed, int failed, const std::vector<test_result>& results) {
			std::println("\n========================================");
			std::println("Test Results");
			std::println("========================================");

			for (const auto& result : results) {
				if (result.passed) {
					std::println("✓ {}", result.test_name);
				} else {
					std::println("✗ {}", result.test_name);
					for (const auto& failure : result.failures) {
						std::println("  - {}", failure);
					}
				}
			}

			std::println("========================================");
			std::println("Total: {} passed, {} failed", passed, failed);
			std::println("========================================\n");
		}
	};

	thread_local test_result* test_registry::current_test_context = nullptr;

	inline void record_assertion_failure(std::string_view message) {
		if (test_registry::current_test_context) {
			test_registry::current_test_context->failures.push_back(std::string(message));
		}
	}
}
