export module gs:task;

import std;
import :channel;

// forward declaration for use in promise
template<typename, bool> class task;

template <typename PromiseType>
struct final_awaiter {
	std::atomic_int64_t* on_done = nullptr;
	bool await_ready() noexcept {
		return false;
	}
	auto await_suspend(std::coroutine_handle<PromiseType> h) noexcept {
		if (on_done) {
			on_done->fetch_add(1);
			on_done->notify_one();
		}
		return h.promise().continuation;
	}
	void await_resume() noexcept {}
};

// Promise implementation used by task.
template<typename ValueType, bool InitialSuspend>
struct task_promise_base {
    using value_type = ValueType;
	std::exception_ptr exception;
    std::coroutine_handle<> continuation = std::noop_coroutine();
	std::atomic_int64_t ref{ 0 };
	std::atomic_int64_t* on_done = nullptr;

	auto initial_suspend() noexcept {
		if constexpr (InitialSuspend)
			return std::suspend_always{};
		else
			return std::suspend_never{};
	}
    void unhandled_exception() noexcept { exception = std::current_exception(); }
};

template<typename ValueType, bool InitialSuspend>
struct task_promise : task_promise_base<ValueType, InitialSuspend> {
	std::optional<ValueType> value{};
	
	auto get_return_object() noexcept -> task<ValueType, InitialSuspend>;
	auto final_suspend() noexcept { return final_awaiter<task_promise<ValueType, InitialSuspend>>{this->on_done}; }
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
	auto final_suspend() noexcept { return final_awaiter<task_promise<void,InitialSuspend>>{this->on_done}; }
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
	[[nodiscard]] task() noexcept = default;
    [[nodiscard]] task(task&& other) noexcept : h(std::exchange(other.h, nullptr)) {}
    [[nodiscard]] task(task const& other) noexcept : h(other.h) {
		h.promise().ref += 1;
	}
    [[nodiscard]] explicit task(std::coroutine_handle<promise_type> h) noexcept : h(h) {
		h.promise().ref += 1;
	}
	~task() {
		if (h && (0 == --h.promise().ref)) {
			h.destroy();
		}
	}

	task& operator=(task&& other) noexcept {
		if (h) h.promise().ref--;
        h = std::exchange(other.h, nullptr);
        return *this;
    }

    task& operator=(task const& other) {
		if (h) h.promise().ref--;
		h = other.h;
		if (h) h.promise().ref++;
		return *this;
    }

	// True when the coroutine has run to completion (or there is no coroutine).
	// Used to join fire-and-forget helpers before destroying their handles.
	bool done() const noexcept {
		return !h || h.done();
	}

	void resume() {
		h.resume();
	}

	void on_promise_destroyed(std::atomic_int64_t* i) {
		h.promise().on_done = i;
	}

	// Get the next value from the task, waiting for it to complete if necessary.
	ValueType result() requires(!std::is_void_v<ValueType>) {
		// Keep resuming until I have a value
		while (!h.promise().value) {
			h.resume();
		}
		return await_resume();
	}

	// Awaiter support for co_await.
	// false -> the coroutine is suspended
	// true -> the coroutine is not suspended
	bool await_ready() const noexcept {
		if constexpr (!std::is_void_v< ValueType>) {
			return h.done() && h.promise().value.has_value();
		}
		else {
			return h.done();
		}
	}

	auto await_suspend(std::coroutine_handle<> handle) noexcept {
		h.promise().continuation = handle;
		return h;
	}

	auto await_resume() -> ValueType {
		if (h && h.promise().exception)
			std::rethrow_exception(h.promise().exception);

		if constexpr (!std::is_void_v<ValueType>) {
			if (!h)
				throw std::runtime_error("task is destroyed");
			ValueType vt = std::move(*h.promise().value);
			h.promise().value.reset();
			return vt;
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
