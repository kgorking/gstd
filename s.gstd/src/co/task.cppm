// Implements task coroutine support for CPU-heavy work via thread pool.
export module gs:task;

import std;
import :thread_pool;

template<typename ValueType> class task; // forward declaration for use in promise

// awaiter support
template<typename ValueType, typename PromiseType>
struct awaiter {
    std::coroutine_handle<PromiseType> h;

    bool await_ready() const noexcept {
		// If the task is already suspended, a value is ready.
		// Skips all the coroutine machinery and just returns the value immediately.
		return h.promise().suspended.test();
	}

	auto await_suspend(std::coroutine_handle<PromiseType> current) {
		return current;
	}

    void await_resume() requires (std::is_void_v<ValueType>) {
		auto& promise = h.promise();
		if (promise.exception) std::rethrow_exception(promise.exception);
		promise.suspended.clear();
		promise.suspended.notify_one();
	}

    auto await_resume() -> ValueType requires (!std::is_void_v<ValueType>) {
		auto& promise = h.promise();
		promise.suspended.wait(false);

		if (promise.exception)
            std::rethrow_exception(promise.exception);

		ValueType vt = std::move(promise.value);
		promise.suspended.clear();
		promise.suspended.notify_one();
		return vt;
    }
};

//
// Promise implementation used by task.
// The coroutine submits its result or exception through this object
template<typename ValueType>
struct task_promise_base {
	using value_type = ValueType;
	std::atomic_flag suspended{};
	std::atomic_flag done{};
	std::exception_ptr exception = nullptr;

	// Task are always suspended at the beginning, so we can resume them on the thread pool
	auto initial_suspend() noexcept {
		done.clear();
		auto handle = std::coroutine_handle<task_promise_base>::from_promise(*this);
		thread_pool::instance().enqueue(handle);
		return std::suspend_always{};
	}
	auto final_suspend() noexcept -> std::suspend_always {
		set_done();
		return {};
	}
	auto get_return_object() noexcept -> task<ValueType>;
	void unhandled_exception() noexcept {
		exception = std::current_exception();
		set_done();
	}

	void wait_until_suspended() const noexcept {
		suspended.wait(false);
	}
	void wait_until_not_suspended() const noexcept {
		suspended.wait(true);
	}

	void wait_until_done() const noexcept {
		done.wait(false);
	}

	void set_done() noexcept {
		done.test_and_set();
		done.notify_one();
	}

	void set_suspended() noexcept {
		suspended.test_and_set();
		suspended.notify_one();
	}
};

template<typename ValueType>
struct task_promise : task_promise_base<ValueType> {
	ValueType value{};

	auto get_return_object() noexcept -> task<ValueType>;

	auto yield_value(ValueType&& v) noexcept {
		this->wait_until_not_suspended();
		value = std::forward<ValueType>(v);
		this->set_suspended();
		return std::suspend_never{}; // let it rip on the scheduled thread
	}
	void return_value(ValueType&& v) noexcept {
		this->wait_until_not_suspended();
		value = std::forward<ValueType>(v);
		this->set_suspended();
		this->set_done();
	}
};

template<>
struct task_promise<void> : task_promise_base<void> {
	auto get_return_object() noexcept -> task<void>;
	void return_void() noexcept {
		this->set_done();
	}
};

export template<typename ValueType = void>
class task {
public:
    using promise_type = task_promise<ValueType>;
    using value_type = ValueType;

private:
    std::coroutine_handle<promise_type> _handle = nullptr;

public:
    // constructors / destructor
    task() noexcept = default;
    explicit task(std::coroutine_handle<promise_type> h) noexcept : _handle(h) {}
    task(task&& other) noexcept : _handle(other._handle) { other._handle = nullptr; }
    task(const task&) = delete;
	auto operator=(task&&) = delete;

    bool done() const noexcept { return !_handle || _handle.promise().done.test(); }

	void wait() const {
		if (!done()) {
			_handle.promise().wait_until_done();
		}
	}

	template<typename T = ValueType>
    T result() const requires (!std::is_void_v<T>) {
		// Wait until a value is ready
		auto& suspended = _handle.promise().suspended;
		suspended.wait(false);

		// Snag the value, and signal that we're done consuming it
		T value = std::move(_handle.promise().value);
		suspended.clear();
		suspended.notify_one();

		return value;
    }

    auto operator co_await() & noexcept {
        return awaiter<ValueType, promise_type>{_handle};
    }

    auto operator co_await() && noexcept {
        return awaiter<ValueType, promise_type>{std::exchange(_handle, nullptr)};
    }
};

// out-of-line definitions now that 'task' is complete

template<typename ValueType>
auto task_promise<ValueType>::get_return_object() noexcept -> task<ValueType> {
	auto handle = std::coroutine_handle<task_promise>::from_promise(*this);
    return task<ValueType>{handle};
}

auto task_promise<void>::get_return_object() noexcept -> task<void> {
	auto handle = std::coroutine_handle<task_promise>::from_promise(*this);
    return task<void>{handle};
}

// Utility to wait for multiple tasks and collect their results
export template<typename... ValueTypes>
auto wait_all(task<ValueTypes>&... tasks) {
	return std::make_tuple(tasks.result()...);
}
