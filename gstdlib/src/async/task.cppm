export module gs:task;

import std;
import :channel;

// forward declaration for use in promise
template<typename> class task;

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
template<typename ValueType>
struct task_promise_base {
    using value_type = ValueType;
	std::exception_ptr exception;
    std::coroutine_handle<> continuation = nullptr;

	auto initial_suspend() noexcept		{ return std::suspend_never{}; }
    void unhandled_exception() noexcept { exception = std::current_exception(); }
};

template<typename ValueType>
struct task_promise : task_promise_base<ValueType> {
	std::optional<ValueType> value{};
	
	auto get_return_object() noexcept -> task<ValueType>;
	auto final_suspend() noexcept { return final_awaiter<task_promise<ValueType>>{}; }
	auto yield_value(ValueType&& v) noexcept -> std::suspend_always {
		this->value = std::forward<ValueType>(v);
		return {};
	}
    void return_value(ValueType&& v) noexcept {
		this->value = std::forward<ValueType>(v);
	}
};

template<>
struct task_promise<void> : task_promise_base<void> {
	auto get_return_object() noexcept -> task<void>;
	auto final_suspend() noexcept { return final_awaiter<task_promise<void>>{}; }
	void return_void() noexcept { }
};

export template<typename ValueType = void>
class task {
public:
    using promise_type = task_promise<ValueType>;
    using value_type = ValueType;

private:
    std::coroutine_handle<promise_type> h = nullptr;
	bool copy = false;

public:
    task() noexcept = default;
    task(task&& other) noexcept : h(std::exchange(other.h, nullptr)) { }
    task(task const& other) noexcept : h(other.h), copy(true) {}
    explicit task(std::coroutine_handle<promise_type> h) noexcept : h(h) {}
	~task() {
		if (!copy)
			h.destroy();
	}

	task& operator=(task&& other) noexcept {
        h = std::exchange(other.h, nullptr);
        return *this;
    }

    task& operator=(task const& other) {
        h = other.h;
		copy = true;
        return *this;
    }

	// Get the next value from the task, waiting for it to complete if necessary.
	ValueType result() requires(!std::is_void_v<ValueType>) {
		if (!h.promise().value) {
			if (!h || h.done())
				throw std::runtime_error("task is not valid or already completed");
			h.resume();
		}
		return await_resume();
	}

	// Awaiter support for co_await.
	// false -> the coroutine is suspended
	// true -> the coroutine is not suspended
	bool await_ready() const noexcept {
		if constexpr (!std::is_void_v< ValueType>) {
			return h.promise().value.has_value();
		}
		else {
			return false;
		}
	}

	bool await_suspend(std::coroutine_handle<> /*handle*/) noexcept {
		//h.promise().continuation = handle;
		//if (!h.done())
			h.resume();

		// true returns control to the caller/resumer of the current coroutine
		// false resumes the current coroutine.
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

template<typename ValueType>
auto task_promise<ValueType>::get_return_object() noexcept -> task<ValueType> {
    auto handle = std::coroutine_handle<task_promise>::from_promise(*this);
    return task<ValueType>{handle};
}

auto task_promise<void>::get_return_object() noexcept -> task<void> {
    auto handle = std::coroutine_handle<task_promise>::from_promise(*this);
    return task<void>{handle};
}
