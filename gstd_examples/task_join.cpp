import gs;
import std;

using namespace std::chrono_literals;

static task<int, true> sleep_task(int64 ms) {
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
	co_return 1;
}

static void run_benchmark(std::string_view label, frame_cache_mode mode, int iterations) {
	frame_cache::set_mode(mode);
	frame_cache::reset_stats();
	frame_cache::clear();

	uint64 counter = 0;
	auto const start = std::chrono::system_clock::now();
	for (int i = 0; i < iterations; i++) {
		auto tasks = std::array{
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0),
			sleep_task(0)
		};

		for (int result : yield_all(std::move(tasks)))
			counter += result;
	}

	auto const end = std::chrono::system_clock::now();
	auto const diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	auto const diff_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
	auto const s = frame_cache::snapshot();
	std::println("[{}] time: {:L} ms for {:L} tasks, {:.2f} ns/task, {:.2f} ms/task | allocs={:L} hits_tls={:L} hits_shared={:L} miss={:L} hit_rate={:.1f}% frees={:L} (tls={:L} shared={:L} dropped={:L})",
		label, diff_ms.count(), counter, diff_ns.count() / double(counter ? counter : 1), diff_ms.count() / double(counter ? counter : 1),
		s.alloc_total, s.alloc_hit_tls, s.alloc_hit_shared, s.alloc_miss, s.hit_rate() * 100.0,
		s.free_total, s.free_to_tls, s.free_to_shared, s.free_dropped);
}

int main(int argc, char** argv) {
	// Set the global locale to US English with UTF-8 encoding
	std::locale::global(std::locale("da_DK.UTF-8"));
	std::cout.imbue(std::locale());

	// Initialize thread pool
	std::ignore = thread_pool::instance();
	std::this_thread::sleep_for(10ms);

	int iterations = 100000;
	if (argc > 1)
		iterations = std::atoi(argv[1]);
	std::println("iterations per mode: {} (pass count as argv[1])", iterations);

	// A/B: baseline first, then TLS-only (Strategy 1), shared-only
	// (Strategy 2), hybrid. Stats reset + cache cleared per mode.
	//run_benchmark("disabled   ", frame_cache_mode::disabled, iterations);
	//run_benchmark("tls_only   ", frame_cache_mode::tls_only, iterations);
	//run_benchmark("shared_only", frame_cache_mode::shared_only, iterations);
	run_benchmark("hybrid     ", frame_cache_mode::hybrid, iterations);

	auto const s = frame_cache::snapshot();
	std::println("\nsizes allocation counts:");
	for (auto const [k, v] : s.sizes_count) {
		std::println(" size: {:5} x {}", k, v);
	}
}
