module;

#include <version>

export module gs:registry;

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
			void (*run)();
		};

		static test_registry& instance() {
			static test_registry reg;
			return reg;
		}

		static std::string location_name(std::source_location const& loc) {
			std::string_view file = loc.file_name();
			if (auto const pos = file.find_last_of("/\\"); pos != std::string_view::npos)
				file.remove_prefix(pos + 1);

			return std::format("{}:{}", file, loc.line());
		}

		std::map<std::filesystem::path, std::vector<test_entry>> tests;
		std::map<std::filesystem::path, std::vector<test_result>> results;
		std::mutex registry_mutex;

	public:
		constexpr static void register_test(auto test_fn, std::source_location loc) {
			std::string lambda_name = location_name(loc);
			instance().tests[loc.file_name()].emplace_back(test_entry{ std::move(lambda_name), loc.line(), test_fn });
		}

		static void run_all_tests() {
			auto& reg = instance();
			reg.results.clear();

			int passed = 0;
			int failed = 0;

			for (const auto& [path, tests] : reg.tests) {
				std::println("{}", path.filename().stem().generic_string());

				std::for_each (std::execution::seq, tests.begin(), tests.end(), [&](const auto& entry) {
					test_result result;
					result.test_name = entry.name;
					result.source_line = entry.source_line;

					try {
						// Set current test context for assertions
						current_test_context = &result;

						// Run the test
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

					std::lock_guard<std::mutex> lock(reg.registry_mutex);
					if (!result.passed) {
						std::println("\t\033[31m✗\033[0m {}", result.test_name);
						for (const auto& failure : result.failures) {
							std::println("\t  - {}", failure);
						}
					}
					/*else {
						std::println("\t\033[32m✓\033[0m {}", result.test_name);
					}*/

					current_test_context = nullptr;
					reg.results[path].push_back(result);
				});
			}

			std::println("Total: {} passed, {} failed", passed, failed);
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
