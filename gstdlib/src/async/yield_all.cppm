export module gs:yield_all;

import std;
import :channel;
import :sequence;
import :task;
import :thread_pool;

template<typename ValueType>
task<void> helper(task<ValueType> t, channel<ValueType>& ch) {
	co_await thread_pool::switch_to_thread();
	ch.set(co_await t);
};

export template<typename ValueType, int N>
auto yield_all(std::array<task<ValueType>, N> tasks) -> sequence<ValueType>
{
	channel<ValueType> ch;
	std::array<task<void>, N> helpers{};
	for (int i = 0; i < N; ++i) helpers[i] = helper(tasks[i], ch);
	for (int n = 0; n < N; ++n) co_yield ch.get();
}
