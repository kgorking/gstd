export module gs:yield_all;

import std;
import :channel;
import :task;
import :thread_pool;
import :sequence;

template<typename R>
concept awaitable_range = std::ranges::input_range<R> && requires(std::ranges::range_value_t<R> t) { t.await_ready(); };

/*export template<int Buffer = 16>
auto yield_all(awaitable_range auto tasks) -> sequence<typename std::ranges::range_value_t<decltype(tasks)>::value_type>
{
	using TaskType = std::ranges::range_value_t<decltype(tasks)>;
	using ValueType = typename TaskType::value_type;
	channel<ValueType, Buffer> ch;

	auto helper = [&ch](TaskType t) -> task<void> {
		co_await thread_pool::switch_to_thread();
		ch << co_await t;
		};

	auto helpers = tasks | std::views::transform(helper) | std::ranges::to<std::vector>();

	for (int n = 0; n < tasks.size(); ++n)
		co_yield ch.get();
}*/

export template<typename ValueType, int N>
auto yield_all(std::array<task<ValueType>, N> tasks) -> sequence<ValueType>
{
	channel<ValueType, N> ch;
	//std::latch all_done{N};

	auto helper = [&ch/*, &all_done*/](task<ValueType> t) -> task<void> {
		co_await thread_pool::switch_to_thread();
		ch << co_await t;
		//all_done.count_down();
	};

	std::array<task<void>, N> helpers{};
	std::ranges::transform(tasks, helpers.begin(), helper);

	for (int n = 0; n < N; ++n) {
		co_yield ch.get();
	}

	// Prevent this coroutine from exiting before all helper tasks have completed,
	// which could lead to undefined behavior if the channel is destroyed while helpers are still running
	// and writing to the channel.
//	all_done.wait();
}
