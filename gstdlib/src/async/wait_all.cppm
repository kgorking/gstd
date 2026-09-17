export module gs:wait_all;

import std;
import :channel;
import :task;
import :thread_pool;

export template<typename... ValueTypes>
auto wait_all(task<ValueTypes>... tasks) -> std::tuple<ValueTypes...>
{
	channel<std::tuple<ValueTypes...>> ch;

	auto resume_on_thread = [&]() -> task<void> {
		co_await thread_pool::switch_to_thread();
		ch << std::make_tuple(co_await tasks...);
		}();

	return ch.get();
}
