export module gs:task;

import std;
import :channel;

// forward declaration for use in promise
template<typename, bool> class task;

template <typename PromiseType>
struct final_awaiter {
	bool await_ready() noexcept { return false; }
	std::coroutine_handle<> await_suspend(std::coroutine_handle<PromiseType> h) noexcept {
		// Resume the continuation directly instead of calling handle.resume()
		// recursively from inside the current frame -- the compiler compiles
		// this into a tail call, so a long chain of co_await'd tasks never
		// blows the stack.
		auto continuation = h.promise().continuation;
		return continuation ? continuation : std::noop_coroutine();
	}
	void await_resume() noexcept {}
};

// Promise implementation used by task.
template<typename ValueType, bool InitialSuspend>
struct task_promise_base {
    using value_type = ValueType;
	std::exception_ptr exception;
    std::coroutine_handle<> continuation = nullptr;

	auto initial_suspend() noexcept		{ if constexpr (InitialSuspend) return std::suspend_always{}; else return std::suspend_never{}; }
    void unhandled_exception() noexcept { exception = std::current_exception(); }
};

template<typename ValueType, bool InitialSuspend>
struct task_promise : task_promise_base<ValueType, InitialSuspend> {
	std::optional<ValueType> value{};
	
	auto get_return_object() noexcept -> task<ValueType, InitialSuspend>;
	auto final_suspend() noexcept { return final_awaiter<task_promise<ValueType, InitialSuspend>>{}; }
	auto yield_value(ValueType&& v) noexcept -> std::suspend_always {
		this->value = std::forward<ValueType>(v);
		return {};
	}
    void return_value(ValueType&& v) noexcept {
		this->value = std::forward<ValueType>(v);
	}
};

template<bool InitialSuspend>
struct task_promise<void, InitialSuspend> : task_promise_base<void, InitialSuspend> {
	auto get_return_object() noexcept -> task<void, InitialSuspend>;
	auto final_suspend() noexcept { return final_awaiter<task_promise<void, InitialSuspend>>{}; }
	void return_void() noexcept { }
};

export template<typename ValueType = void, bool InitialSuspend = false>
class task {
public:
    using promise_type = task_promise<ValueType, InitialSuspend>;
    using value_type = ValueType;

private:
    std::coroutine_handle<promise_type> h = nullptr;

public:
    task() noexcept = default;
    task(task&& other) noexcept : h(std::exchange(other.h, nullptr)) { }
    task(task const& other) noexcept : h(other.h) {}
    explicit task(std::coroutine_handle<promise_type> h) noexcept : h(h) {}

	task& operator=(task&& other) noexcept {
        h = std::exchange(other.h, nullptr);
        return *this;
    }

    task& operator=(task const& other) {
        h = other.h;
        return *this;
    }

	// Wait for the task to complete, re-throwing any exception that occurred.
	void wait() {
		while (!done()) {
			h.resume();
		}
	}

	void resume() {
		if (h && !h.done())
			h.resume();
	}

	// Get the next value from the task, waiting for it to complete if necessary.
	ValueType result() requires(!std::is_void_v<ValueType>) {
		if (h && !h.done())
			h.resume();
		return await_resume();
	}

	bool done() const noexcept {
        return !h || h.done();
    }

	// Awaiter support for co_await.
	// Each co_await drives the awaited task forward by one step
	// (up to its next co_yield or co_return) and then continues
	// the awaiting coroutine without suspending, so yielded values
	// are passed back correctly.
	bool await_ready() const noexcept {
		return !h || h.done();
	}

	bool await_suspend(std::coroutine_handle<>) noexcept {
		if constexpr (!std::is_void_v< ValueType>) {
			if (h.promise().value) {
				return true;
			}
		}

		h.resume();
		return false;
	}

	auto await_resume() -> ValueType {
		if (h.promise().exception)
			std::rethrow_exception(h.promise().exception);
		if constexpr (!std::is_void_v< ValueType>) {
			return std::exchange(h.promise().value, std::nullopt).value();
		}
	}
};

template<typename ValueType, bool InitialSuspend>
auto task_promise<ValueType, InitialSuspend>::get_return_object() noexcept -> task<ValueType, InitialSuspend> {
    auto handle = std::coroutine_handle<task_promise>::from_promise(*this);
    return task<ValueType, InitialSuspend>{handle};
}

template<bool InitialSuspend>
auto task_promise<void, InitialSuspend>::get_return_object() noexcept -> task<void, InitialSuspend> {
    auto handle = std::coroutine_handle<task_promise>::from_promise(*this);
    return task<void, InitialSuspend>{handle};
}

export void wait_all(std::ranges::range auto&& tasks)
{
    for (auto& task : tasks)
        task.wait();
}