// Implements task coroutine support for CPU-heavy work via thread pool.
export module gs:task;

import std;
import :channel;
import :thread_pool;
import :sequence;

export template<typename ValueType> class task; // forward declaration for use in promise

static channel<int> task_completion_signal;

// awaiter support - schedules task on thread pool when awaited
template<typename ValueType, typename PromiseType>
struct awaiter {
    std::coroutine_handle<PromiseType> h;

    bool await_ready() const noexcept { return !h || h.done(); }
    
    bool await_suspend(std::coroutine_handle<> /*cont*/)  {
        // If task is already done, no suspension needed
        if (h && h.done())
            return false;

        // Wait for task to complete before returning control
        h.promise().is_running.wait(true);
        
        // Don't suspend - the task is now complete and the value is ready
        return false;
    }
    
    // Different return types based on ValueType
    void await_resume() noexcept requires (std::is_void_v<ValueType>) {
        if (h && h.promise().exception)
            std::rethrow_exception(h.promise().exception);
    }
    
    ValueType await_resume() noexcept requires (!std::is_void_v<ValueType>) {
        if (h && h.promise().exception)
            std::rethrow_exception(h.promise().exception);
        // return stored return value, or default-constructed if absent
        if (h && h.promise().returned_value.has_value())
            return *h.promise().returned_value;
        return ValueType{};
    }
};

template<typename PromiseType>
struct final_awaiter {
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<PromiseType> h) noexcept {
        h.promise().is_running = false;
        h.promise().is_running.notify_one();
        if (h.promise().task_id) {
            task_completion_signal << *h.promise().task_id;
        }

        if (auto cont = h.promise().continuation)
            cont.resume();
    }
    void await_resume() const noexcept {}
};

// promise implementation used by task; runs on thread pool
template<typename ValueType>
struct task_promise_base {
    using value_type = ValueType;
    std::coroutine_handle<> continuation = nullptr;
    std::exception_ptr exception = nullptr;
    std::atomic<bool> is_running = false;
    std::optional<int> task_id{};

    auto get_return_object() noexcept -> task<ValueType>;
    // Suspend initially so we can schedule on thread pool
    auto initial_suspend() noexcept { return std::suspend_always{}; }
    
    void unhandled_exception() noexcept {
        exception = std::current_exception();
    }
};

template<typename ValueType>
struct task_promise : task_promise_base<ValueType> {
    std::optional<ValueType> returned_value;

    auto final_suspend() noexcept -> final_awaiter<task_promise> { return {}; }
    auto get_return_object() noexcept -> task<ValueType>;
    void return_value(ValueType&& v) noexcept {
        returned_value = std::forward<ValueType>(v);
    }
};

// Promise for void-returning task coroutines
template<>
struct task_promise<void> : task_promise_base<void> {
    auto final_suspend() noexcept -> final_awaiter<task_promise> { return {}; }
    auto get_return_object() noexcept -> task<void>;
    void return_void() noexcept {}
};

export template<typename ValueType = void>
class task {
public:
    using promise_type = task_promise<ValueType>;
    using value_type = ValueType;

private:
    std::coroutine_handle<promise_type> _handle = nullptr;
    int _id = 0;
    inline static std::atomic<int> _id_counter{1};

public:
    // constructors / destructor
    task() noexcept = default;
    explicit task(std::coroutine_handle<promise_type> h) noexcept : _handle(h) {
        if (h) {
            _id = _id_counter++;
        }
    }
    task(task&& other) noexcept : _handle(other._handle), _id(other._id) { other._handle = nullptr; }
    task& operator=(task&& other) noexcept {
        if (this != &other) {
            if (_handle)
                _handle.destroy();
            _handle = other._handle;
            _id = other._id;
            other._handle = nullptr;
        }
        return *this;
    }
    task(const task&) = delete;
    task& operator=(const task&) = delete;

    ~task() {
        if (_handle)
            _handle.destroy();
    }

    int id() const noexcept { return _id; }

    // Get the current status
    bool done() const noexcept { return !_handle || _handle.done(); }
    
    void resume() noexcept {
        if (_handle && !_handle.done())
            _handle.resume();
    }
   
    void signal_on_completion() noexcept {
        if (_handle && !_handle.done())
            _handle.promise().task_id = _id;
    }

    // Execute on thread pool and wait for completion
    void wait() const noexcept {
        if (_handle && !_handle.done() && _handle.promise().is_running) {
            // Wait until the coroutine completes
            _handle.promise().is_running.wait(true);

            // If there was an exception, rethrow it
            if (_handle.promise().exception)
                std::rethrow_exception(_handle.promise().exception);
        }
    }

    // Get the result of the task, if it has one.
    // Returns std::nullopt if the task threw an exception or if the result is not available.
    // Blocks until the task is complete.
    template<typename T = ValueType>
    T result() const noexcept requires (!std::is_void_v<T>) {
        wait();
        return std::move(*_handle.promise().returned_value);
    }

    // lvalue overload: the caller retains ownership of the handle
    auto operator co_await() & noexcept {
        return awaiter<ValueType, promise_type>{_handle};
    }

    // rvalue overload: transfer ownership to the awaiter so that the
    // temporary object won't destroy the handle before the coroutine
    // finishes
    auto operator co_await() && noexcept {
        return awaiter<ValueType, promise_type>{std::exchange(_handle, nullptr)};
    }
};

// out-of-line definitions now that 'task' is complete

template<typename ValueType>
auto task_promise<ValueType>::get_return_object() noexcept -> task<ValueType> {
    auto handle = std::coroutine_handle<task_promise>::from_promise(*this);
    handle.promise().is_running = true;
    thread_pool::instance().enqueue(handle);
    return task<ValueType>{handle};
}

auto task_promise<void>::get_return_object() noexcept -> task<void> {
    auto handle = std::coroutine_handle<task_promise>::from_promise(*this);
    handle.promise().is_running = true;
    thread_pool::instance().enqueue(handle);
    return task<void>{handle};
}

// Utility to wait for multiple tasks and collect their results
export template<typename... Tasks>
auto wait_all(Tasks&... tasks) {
    return std::make_tuple(tasks.result()...);
}

export template<template<typename, auto...> typename Container, typename T, auto... Rest>
requires (Span<Container<task<T>, Rest...>, task<T>>)
sequence<T> wait_each(Container<task<T>, Rest...>& tasks) {
    std::map<int, task<T>*> task_map;
    for (task<T>& task : tasks){
        if (!task.done()) {
            task.signal_on_completion();
            task_map[task.id()] = &task;
        }
    }

    auto remaining = tasks.size();

    while (remaining > 0) {
        int completed_task_id = task_completion_signal.get();
        if (task_map.contains(completed_task_id)) {
            --remaining;
            task<T>* completed_task = task_map[completed_task_id];
            task_map.erase(completed_task_id);
            co_yield completed_task->result();
        }
    }
}

export template<typename... Args>
auto as_task(auto&& fn, Args&&... args) -> task<std::invoke_result_t<decltype(fn), Args...>> {
    co_return fn(std::forward<Args>(args)...);
}
