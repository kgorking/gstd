export module gs:yield_all;

import std;
import :channel;
import :sequence;
import :task;
import :thread_pool;

struct scope_counter {
	std::atomic_int64_t& sc;
	std::atomic<bool>& done;

	~scope_counter() noexcept {
		if (0 == --sc) {
			done = true;
			done.notify_one();
		}
	}

	operator bool() const {
		return false == done;
	}
};
struct scope_reference {
	std::atomic_int64_t count{ 1 };
	std::atomic<bool> done{ false };

	~scope_reference() noexcept {
		if (0 != --count) {
			done.wait(false);
		}
	}

	auto get() {
		count += 1;
		return scope_counter{ count, done };
	}
};

template<typename ValueType, typename Slots>
task<void> await_on_thread_v2(task<ValueType> t, std::atomic_int64_t& current_slot, Slots& result_dest, scope_reference& ref) {
	// Make sure yield_all's local variables do not go out of scope
	auto scope = ref.get();

	// Switch to a worker thread
	co_await thread_pool::switch_to_thread();

	ValueType v = co_await t;
	int64 const idx = current_slot.fetch_add(1, std::memory_order_relaxed); // it's whatevs
	result_dest[idx].v = std::move(v);
	result_dest[idx].flag = true;
	result_dest[idx].flag.notify_one();
};

export template<typename ValueType, int N>
auto yield_all(std::array<task<ValueType>, N> const& tasks) -> sequence<ValueType>
{
#pragma warning(disable : 4324)
	// Avoid false sharing between threads.
	struct alignas(64) val_wrap {
		ValueType v{};
		std::atomic_bool flag{ false };
	};

	scope_reference ref;
	std::array<val_wrap, N> results{};
	std::array<task<void>, N> helpers{};
	std::atomic_int64_t current_slot{ 0 };

	// Set up the helper tasks
	for (int i = 0; i < N; ++i)
		helpers[i] = await_on_thread_v2(std::move(tasks[i]), current_slot, results, ref);

	for (int n = 0; n < N; ++n) {
		results[n].flag.wait(false);
		co_yield results[n].v;
	}
}

/* Works, but is slower
export module gs:yield_all;

import std;
import :channel;
import :sequence;
import :task;
import :thread_pool;

template<typename ValueType>
task<void> await_on_thread(task<ValueType> t, channel<ValueType>& ch) {
	co_await thread_pool::switch_to_thread();
	ch.set(co_await t);
};

export template<typename ValueType, int N>
auto yield_all(std::array<task<ValueType>, N> tasks) -> sequence<ValueType>
{
	channel<ValueType> ch;
	std::array<task<void>, N> helpers{};
	for (int i = 0; i < N; ++i) helpers[i] = await_on_thread(tasks[i], ch);
	for (int n = 0; n < N; ++n) co_yield ch.get();
}
*/