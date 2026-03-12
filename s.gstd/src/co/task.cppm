// Implements task coroutine support for CPU-heavy work via thread pool.
export module gs:task;

import std;
import :thread_pool;

export template<typename ValueType> class task; // forward declaration for use in promise

// promise implementation used by task; runs on thread pool
template<typename ValueType>
struct task_promise {
    std::coroutine_handle<> continuation = nullptr;
    std::optional<ValueType> returned_value;
    std::exception_ptr exception = nullptr;
    bool is_running = false;

    auto get_return_object() noexcept -> task<ValueType>;
    
    // Don't suspend on entry - start execution immediately on thread pool
    auto initial_suspend() noexcept { return std::suspend_never{}; }
    
    struct final_awaiter {
        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<task_promise> h) noexcept {
            if (auto cont = h.promise().continuation)
                cont.resume();
        }
        void await_resume() const noexcept {}
    };
    
    auto final_suspend() noexcept -> final_awaiter { return {}; }

    void return_value(ValueType v) noexcept {
        returned_value = v;
    }
    
    void unhandled_exception() noexcept {
        exception = std::current_exception();
    }
};

// Promise for void-returning task coroutines
template<>
struct task_promise<void> {
    std::coroutine_handle<> continuation = nullptr;
    std::exception_ptr exception = nullptr;
    bool is_running = false;

    auto get_return_object() noexcept -> task<void>;
    
    // Don't suspend on entry - start execution immediately on thread pool
    auto initial_suspend() noexcept { return std::suspend_never{}; }
    
    struct final_awaiter {
        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<task_promise> h) noexcept {
            if (auto cont = h.promise().continuation)
                cont.resume();
        }
        void await_resume() const noexcept {}
    };
    
    auto final_suspend() noexcept -> final_awaiter { return {}; }
    
    void return_void() noexcept {}
    
    void unhandled_exception() noexcept {
        exception = std::current_exception();
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
    task& operator=(task&& other) noexcept {
        if (this != &other) {
            if (_handle)
                _handle.destroy();
            _handle = other._handle;
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

    // Get the current status
    bool done() const noexcept { return !_handle || _handle.done(); }
    
    void resume() noexcept {
        if (_handle && !_handle.done())
            _handle.resume();
    }
    
    // Schedule execution on thread pool if not already scheduled
    void schedule() noexcept {
        if (_handle && !_handle.done()) {
            if (!_handle.promise().is_running) {
                _handle.promise().is_running = true;
                thread_pool::instance().enqueue(_handle);
            }
        }
    }
    
    // Execute on thread pool and wait for completion
    task& wait() noexcept {
        if (_handle && !_handle.done()) {
            schedule();
            
            // Wait until the coroutine completes
            while (!_handle.done()) {
                std::this_thread::yield();
            }
        }
        return *this;
    }

    // return value retrieval (only for non-void types)
    template<typename T = ValueType>
    std::optional<T> result() const noexcept requires (!std::is_void_v<T>) {
        if (_handle && _handle.promise().exception)
            return std::nullopt;
        return _handle.promise().returned_value;
    }

    // awaiter support - schedules task on thread pool when awaited
    struct awaiter {
        std::coroutine_handle<promise_type> h;

        bool await_ready() const noexcept { return !h || h.done(); }
        
        void await_suspend(std::coroutine_handle<> cont) noexcept {
            h.promise().continuation = cont;
            
            // If not already running, schedule on thread pool
            if (!h.promise().is_running) {
                h.promise().is_running = true;
                thread_pool::instance().enqueue(h);
            }
            // Return control and let thread pool work in background
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

    // lvalue overload: the caller retains ownership of the handle
    auto operator co_await() & noexcept {
        return awaiter{_handle};
    }

    // rvalue overload: transfer ownership to the awaiter so that the
    // temporary object won't destroy the handle before the coroutine
    // finishes
    auto operator co_await() && noexcept {
        awaiter a{_handle};
        _handle = nullptr;
        return a;
    }
};

// out-of-line definitions now that 'task' is complete

template<typename ValueType>
auto task_promise<ValueType>::get_return_object() noexcept -> task<ValueType> {
    return task<ValueType>{std::coroutine_handle<task_promise>::from_promise(*this)};
}

auto task_promise<void>::get_return_object() noexcept -> task<void> {
    return task<>{std::coroutine_handle<task_promise>::from_promise(*this)};
}

// Utility to wait for multiple tasks and collect their results
export template<typename... Tasks>
auto wait_all(Tasks&... tasks) {
    (tasks.wait(), ...);
    return std::make_tuple(tasks.result()...);
}
