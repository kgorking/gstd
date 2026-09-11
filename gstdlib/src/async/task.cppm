// Implements task coroutine support for CPU-heavy work via thread pool.
export module gs:task;

import std;

// forward declaration for use in promise
template<typename ValueType> class task;

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
	std::promise<ValueType> prom;
    std::coroutine_handle<> continuation = nullptr;

	auto initial_suspend() noexcept		{ return std::suspend_always{}; }
    void unhandled_exception() noexcept { prom.set_exception(std::current_exception()); }
};

template<typename ValueType>
struct task_promise : task_promise_base<ValueType> {
    auto get_return_object() noexcept -> task<ValueType>;
	auto final_suspend() noexcept { return final_awaiter<task_promise<ValueType>>{}; }
	auto yield_value(ValueType v) noexcept -> std::suspend_always { this->prom.set_value(std::move(v)); return {}; }
    void return_value(ValueType v) noexcept { this->prom.set_value(std::move(v)); }
};

template<>
struct task_promise<void> : task_promise_base<void> {
	auto get_return_object() noexcept -> task<void>;
	auto final_suspend() noexcept { return final_awaiter<task_promise<void>>{}; }
	void return_void() noexcept { this->prom.set_value(); }
};

export template<typename ValueType = void>
class task {
public:
    using promise_type = task_promise<ValueType>;
    using value_type = ValueType;

private:
    std::coroutine_handle<promise_type> _handle = nullptr;
	std::shared_future<ValueType> fut;

public:
    task() noexcept = default;
    explicit task(std::coroutine_handle<promise_type> h, std::shared_future<ValueType> f) noexcept : _handle(h), fut(std::move(f)) {}
    task(task&& other) noexcept : _handle(other._handle), fut(std::move(other.fut)) { other._handle = nullptr; }
    task(task const& other) : _handle(other._handle), fut(other.fut) {}

	task& operator=(task&& other) noexcept {
        _handle = std::exchange(other._handle, nullptr);
		fut = std::move(other.fut);
        return *this;
    }

    task& operator=(task const& other) {
        _handle = other._handle;
		fut = other.fut;
        return *this;
    }

	void wait() {
		fut.wait();
	}

	ValueType result() requires(!std::is_void_v<ValueType>) {
		_handle.resume();
		return await_resume();
	}

	bool done() const noexcept {
        return !_handle || _handle.done();
    }

	// Awaiter support for co_await
	bool await_ready() const noexcept {
		return _handle.done();
	}

	void await_suspend(std::coroutine_handle<> current) noexcept {
		//std::println("task::await_suspend: current = {}, h = {}", current.address(), _handle.address());
		_handle.promise().continuation = current;
		_handle.resume();
	}

	auto await_resume() -> ValueType {
		return fut.get();
	}
};

template<typename ValueType>
auto task_promise<ValueType>::get_return_object() noexcept -> task<ValueType> {
    auto handle = std::coroutine_handle<task_promise>::from_promise(*this);
    return task<ValueType>{handle, this->prom.get_future().share()};
}

auto task_promise<void>::get_return_object() noexcept -> task<void> {
    auto handle = std::coroutine_handle<task_promise>::from_promise(*this);
    return task<void>{handle, this->prom.get_future().share()};
}

export template<typename... ValueTypes>
auto wait_all(task<ValueTypes>... tasks) -> std::tuple<ValueTypes...>
{
	return std::make_tuple(tasks.result()...);
}

export void wait_all(std::ranges::range auto&& tasks)
{
    for (auto& task : tasks)
        task.wait();
}