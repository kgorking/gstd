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
        auto& promise = h.promise();
        return promise.done.load(std::memory_order_acquire)
            || promise.ready.load(std::memory_order_acquire);
    }

    bool await_suspend(std::coroutine_handle<> current) noexcept {
        auto& promise = h.promise();
        if (promise.done.load(std::memory_order_acquire)
            || promise.ready.load(std::memory_order_acquire)) {
            return false;
        }

        std::lock_guard lock(promise.mutex);
        promise.continuations.push_back(current);
        return true;
    }

    void await_resume() requires (std::is_void_v<ValueType>) {
        auto& promise = h.promise();
        if (promise.exception) std::rethrow_exception(promise.exception);

        std::unique_lock lock(promise.mutex);
        promise.cv.wait(lock, [&] {
            return promise.done.load(std::memory_order_acquire)
                || promise.ready.load(std::memory_order_acquire);
        });

        if (!promise.done.load(std::memory_order_acquire)) {
            promise.ready.store(false, std::memory_order_release);
            promise.cv.notify_all();
        }
    }

    auto await_resume() -> ValueType requires (!std::is_void_v<ValueType>) {
        auto& promise = h.promise();
        if (promise.exception) std::rethrow_exception(promise.exception);

        std::unique_lock lock(promise.mutex);
        promise.cv.wait(lock, [&] {
            return promise.done.load(std::memory_order_acquire)
                || promise.ready.load(std::memory_order_acquire);
        });

        ValueType vt = promise.value;
        if (!promise.done.load(std::memory_order_acquire)) {
            promise.ready.store(false, std::memory_order_release);
            promise.cv.notify_all();
        }
        return vt;
    }
};

// Promise implementation used by task.
template<typename ValueType>
struct task_promise_base {
    using value_type = ValueType;

    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<bool> ready{false};
    std::atomic<bool> done{false};
    std::exception_ptr exception = nullptr;
    std::vector<std::coroutine_handle<>> continuations;

    auto initial_suspend() noexcept {
        done.store(false, std::memory_order_release);
        auto handle = std::coroutine_handle<task_promise_base>::from_promise(*this);
        thread_pool::instance().enqueue(handle);
        return std::suspend_always{};
    }

    auto final_suspend() noexcept -> std::suspend_always {
        {
            std::lock_guard lock(mutex);
            done.store(true, std::memory_order_release);
            ready.store(true, std::memory_order_release);
        }
        cv.notify_all();
        resume_waiters();
        return {};
    }

    void wait_until_done() const noexcept {
        std::unique_lock lock(mutex);
        cv.wait(lock, [this] {
            return done.load(std::memory_order_acquire);
        });
    }

    void resume_waiters() noexcept {
        std::vector<std::coroutine_handle<>> waiters;
        {
            std::lock_guard lock(mutex);
            waiters.swap(continuations);
        }
        for (auto cont : waiters) {
            if (cont) cont.resume();
        }
    }

    void set_done() noexcept {
        {
            std::lock_guard lock(mutex);
            done.store(true, std::memory_order_release);
            ready.store(true, std::memory_order_release);
        }
        cv.notify_all();
        resume_waiters();
    }

    void unhandled_exception() noexcept {
        exception = std::current_exception();
        set_done();
    }
};

template<typename ValueType>
struct task_promise : task_promise_base<ValueType> {
    ValueType value{};

    auto get_return_object() noexcept -> task<ValueType>;

    auto yield_value(ValueType v) noexcept {
        std::unique_lock lock(this->mutex);
        this->cv.wait(lock, [this] {
            return !this->ready.load(std::memory_order_acquire);
        });
        value = std::move(v);
        this->ready.store(true, std::memory_order_release);
        lock.unlock();
        this->cv.notify_all();
        this->resume_waiters();

        std::unique_lock ready_lock(this->mutex);
        this->cv.wait(ready_lock, [this] {
            return !this->ready.load(std::memory_order_acquire);
        });
        return std::suspend_never{};
    }

    void return_value(ValueType v) noexcept {
        {
            std::lock_guard lock(this->mutex);
            value = std::move(v);
            this->done.store(true, std::memory_order_release);
            this->ready.store(true, std::memory_order_release);
        }
        this->cv.notify_all();
        this->resume_waiters();
    }
};

template<>
struct task_promise<void> : task_promise_base<void> {
    auto get_return_object() noexcept -> task<void>;
    void return_void() noexcept {
        {
            std::lock_guard lock(this->mutex);
            this->done.store(true, std::memory_order_release);
            this->ready.store(true, std::memory_order_release);
        }
        this->cv.notify_all();
        this->resume_waiters();
    }
};

export template<typename ValueType = void>
class task {
public:
    using promise_type = task_promise<ValueType>;
    using value_type = ValueType;

private:
    struct handle_state {
        std::coroutine_handle<promise_type> handle = nullptr;
        ~handle_state() {
            if (handle) {
                handle.destroy();
            }
        }
    };

    std::shared_ptr<handle_state> _state;

    std::coroutine_handle<promise_type> handle() const noexcept {
        return _state ? _state->handle : nullptr;
    }

public:
    task() noexcept = default;
    explicit task(std::coroutine_handle<promise_type> h) noexcept
        : _state(std::make_shared<handle_state>()) {
        _state->handle = h;
    }
    task(task&& other) noexcept = default;
    task(task const& other) noexcept = default;
    task& operator=(task&& other) noexcept = default;
    task& operator=(task const& other) noexcept = default;

    bool done() const noexcept {
        auto h = handle();
        return !h || h.promise().done.load(std::memory_order_acquire);
    }

    void wait() const {
        auto h = handle();
        if (!done()) {
            h.promise().wait_until_done();
        }
    }

    template<typename T = ValueType>
    T result() const requires (!std::is_void_v<T>) {
        auto h = handle();
        auto& promise = h.promise();
        {
            std::unique_lock lock(promise.mutex);
            promise.cv.wait(lock, [&] {
                return promise.done.load(std::memory_order_acquire)
                    || promise.ready.load(std::memory_order_acquire);
            });
        }

        T value = promise.value;
        if (!promise.done.load(std::memory_order_acquire)) {
            promise.ready.store(false, std::memory_order_release);
            promise.cv.notify_all();
        }
        return value;
    }

    auto operator co_await() & noexcept {
        return awaiter<ValueType, promise_type>{handle()};
    }

    auto operator co_await() && noexcept {
        return awaiter<ValueType, promise_type>{handle()};
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

export template<typename... ValueTypes>
auto wait_all(task<ValueTypes>... tasks)
    requires requires { (tasks.wait(), ...); }
{
    return std::make_tuple(tasks.result()...);
}

export auto wait_all(std::ranges::range auto&& tasks)
    requires requires { tasks.at(0).wait(); }
{
    for (auto& task : tasks)
        task.wait();
}