export module gs:yield_all;

import std;
import :channel;
import :sequence;
import :task;
import :thread_pool;

template<typename ValueType, int N>
struct yield_all_state {
	// Avoid false sharing between threads.
	struct alignas(64) val_wrap {
		ValueType v{};
		std::atomic_bool flag{ false };
	};

	std::array<val_wrap, N> results{};
	std::atomic_int64_t current_slot{ 0 };
};

template<typename ValueType, bool B, int N>
task<void, true> await_on_thread_v2(task<ValueType, B> t, yield_all_state<ValueType, N>& state) {
	// Switch to a worker thread
	co_await thread_pool::switch_to_thread();

	// Evaluate the input task and store its result in the next
	// available slot in the yield_all's stack.
	ValueType v = co_await t;
	int64 const idx = state.current_slot.fetch_add(1, std::memory_order_relaxed);
	state.results[idx].v = std::move(v);
	state.results[idx].flag = true;
	state.results[idx].flag.notify_one();
};

export template<typename ValueType, bool B, int N>
auto yield_all(std::array<task<ValueType, B>, N>&& tasks) -> sequence<ValueType> {
	auto state = yield_all_state<ValueType, N>{};
	std::array<task<void, true>, N> helpers{};

	// Teardown guard: declared LAST so it is destroyed FIRST on every exit
	// path (normal return AND early destroy via take/break). Blocks until
	// every helper finalized, so a pending/queued helper frame is never
	// destroyed out from under the pool (stale handle -> 0xC0000005).
	struct teardown_guard {
		std::array<task<void, true>, N>* hs;
		std::atomic_int64_t done;
		~teardown_guard() {
			int64 cur = done.load(std::memory_order_acquire);
			while (cur < N) {
				done.wait(cur);
				cur = done.load(std::memory_order_acquire);
			}
			for (auto& hh : *hs) {
				while (!hh.done())
					std::this_thread::yield();
			}
		}
	} teardown{ &helpers };

	for (int i = 0; i < N; ++i) {
		helpers[i] = await_on_thread_v2(std::move(tasks[i]), state);
		helpers[i].on_promise_destroyed(&teardown.done);
		helpers[i].resume();
	}

	// Wait for results and yield them as soon as they arrive
	for (int n = 0; n < N; ++n) {
		state.results[n].flag.wait(false);
		co_yield state.results[n].v;
	}
}
