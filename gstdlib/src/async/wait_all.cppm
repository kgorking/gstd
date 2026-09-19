export module gs:wait_all;

import std;
import :channel;
import :task;
import :thread_pool;

export template<typename... ValueTypes>
auto wait_all(task<ValueTypes>... tasks) -> std::tuple<ValueTypes...>
{
	channel<std::tuple<ValueTypes...>> ch;

	// Initial-suspended: published via schedule() only after full suspension,
	// never from inside await_suspend (see thread_pool::schedule).
	auto resume_on_thread = [&]() -> task<void, true> {
		co_await thread_pool::switch_to_thread();
		ch << std::make_tuple(co_await tasks...);
		}();
	thread_pool::schedule(resume_on_thread);

	auto result = ch.get();
	while (!resume_on_thread.done())
		std::this_thread::yield();
	return result;
}
