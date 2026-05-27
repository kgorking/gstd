// Test registry and runner for the coroutine-based testing framework.
export module gs.testing:registry;

import std;

export namespace gs::testing {
	struct test_result {
		std::string test_name;
		std::uint_least32_t source_line = 0;
		bool passed = true;
		std::vector<std::string> failures;
	};

	class test_registry {
	private:
		struct test_entry {
			std::string name;
			std::uint_least32_t source_line = 0;
			std::function<void()> run;
		};

		static test_registry& instance() {
			static test_registry reg;
			return reg;
		}

		std::map<std::string, std::vector<test_entry>> tests;
		std::map<std::string, std::vector<test_result>> results;
		std::mutex registry_mutex;

	public:
		static void register_test(std::function<void()> test_fn, std::source_location loc) {
			auto trace = std::stacktrace::current();
			auto lambda_name = parse_stacktrace_description_for_name(trace[2].description());
			auto& reg = instance();

			std::lock_guard<std::mutex> lock(reg.registry_mutex);
			reg.tests[loc.file_name()].emplace_back(test_entry{lambda_name, loc.line(), std::move(test_fn)});
		}

		static std::string parse_stacktrace_description_for_name(std::string desc) {
#ifdef _MSC_VER
			// gstd_tests!`dynamic initializer for 'exec_basic_command''+0x5C
			auto start = desc.find("dynamic initializer for '") + 25;
			if (start != std::string::npos) {
				auto end = desc.find('\'', start);
				if (end != std::string::npos && end > start) {
					return desc.substr(start, end - start);
				}
			}
#elifdef __GNUC__
			// test::test<turbo_lussing::{lambda()#1}>(turbo_lussing::{lambda()#1}&&)
			auto start = desc.find("test::test<") + 11;
			if (start != std::string::npos) {
				auto end = desc.find("::", start);
				if (start != std::string::npos && end != std::string::npos && end > start) {
					return desc.substr(start, end - start);
				}
			}
#elifdef __clang__
			// Clang's stacktrace descriptions are not consistent enough to parse reliably
#else
#endif
			return desc;
		}

		static void run_all_tests() {
			auto& reg = instance();
			std::lock_guard<std::mutex> lock(reg.registry_mutex);

			reg.results.clear();
			int passed = 0;
			int failed = 0;

			std::println("\n========================================");
			std::println("Test Results");
			std::println("========================================");

			for (const auto& [filename, tests] : reg.tests) {
				std::println("\n{}", filename);

				for (const auto& entry : tests) {
					test_result result;
					result.test_name = entry.name;
					result.source_line = entry.source_line;

					try {
						// Set current test context for assertions
						current_test_context = &result;
						entry.run();
						if (result.failures.empty()) {
							result.passed = true;
							passed++;
						}
						else {
							result.passed = false;
							failed++;
						}
					}
					catch (std::exception const& e) {
						result.passed = false;
						result.failures.push_back(std::string("Exception: ") + e.what());
						failed++;
					}
					catch (...) {
						result.passed = false;
						result.failures.push_back("Unknown exception");
						failed++;
					}

					if (result.passed) {
						std::println("\t\033[32m✓\033[0m {}", result.test_name);
					}
					else {
						std::println("\t\033[31m✗\033[0m {}", result.test_name);
						for (const auto& failure : result.failures) {
							std::println("\t  - {}", failure);
						}
					}

					current_test_context = nullptr;
					reg.results[filename].push_back(result);
				}
			}

			std::println("\n========================================");
			std::println("Total: {} passed, {} failed", passed, failed);
			std::println("========================================\n");
		}

		static thread_local test_result* current_test_context;
	};

	thread_local test_result* test_registry::current_test_context = nullptr;

	inline void record_assertion_failure(std::string_view message) {
		if (test_registry::current_test_context) {
			test_registry::current_test_context->failures.push_back(std::string(message));
		}
	}
}
