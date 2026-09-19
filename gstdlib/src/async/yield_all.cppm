export module gs:yield_all;

import std;
import :channel;
import :sequence;
import :task;
import :thread_pool;

struct scope_reference {
	std::atomic_int64_t& sc;
	std::atomic<bool>& done;

	~scope_reference() noexcept {
		if (0 == --sc) {
			done = true;
			done.notify_one();
		}
	}

	operator bool() const {
		return false == done;
	}
};
struct scope_counter {
	std::atomic_int64_t count{ 1 };
	std::atomic<bool> done{ false };

	~scope_counter() noexcept {
		if (0 != --count)
			done.wait(false);
	}

	scope_reference get() {
		count += 1;
		return { count, done };
	}
};

template<typename ValueType, typename Slots>
task<void> await_on_thread_v2(task<ValueType>& t, std::atomic_int64_t& current_slot, Slots& result_dest, scope_counter& scnt) {
	// Make sure yield_all's local variables do not go out of scope
	auto ref = scnt.get();

	// Switch to a worker thread
	co_await thread_pool::switch_to_thread();

	// Evaluate the input task and store its result in the next
	// available slot in the yield_all's stack.
	ValueType v = co_await t;
	int64 const idx = current_slot.fetch_add(1, std::memory_order_relaxed);
	result_dest[idx].v = std::move(v);
	result_dest[idx].flag = true;
	result_dest[idx].flag.notify_one();
};

export template<typename ValueType, int N>
auto yield_all(std::array<task<ValueType>, N>& tasks) -> sequence<ValueType> {
	// Avoid false sharing between threads.
	#pragma warning(push)
	#pragma warning(disable : 4324)
	struct alignas(64) val_wrap {
		ValueType v{};
		std::atomic_bool flag{ false };
	};
	#pragma warning(pop)

	std::array<task<void>, N> helpers{};
	std::array<val_wrap, N> results{};
	std::atomic_int64_t current_slot{ 0 };
	scope_counter cnt;

	// Set up the helper tasks
	for (int i = 0; i < N; ++i)
		helpers[i] = await_on_thread_v2(tasks[i], current_slot, results, cnt);

	for (int n = 0; n < N; ++n) {
		results[n].flag.wait(false);
		co_yield results[n].v;
	}
}
