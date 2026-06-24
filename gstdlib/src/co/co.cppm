// Implements coroutine support.
export module gs:co;

import std;

export template<typename ValueType> class co; // forward declaration for use in promise

// awaiter support
template<typename ValueType, typename PromiseType>
struct awaiter {
    std::coroutine_handle<PromiseType> h;

    bool await_ready() const noexcept { return false; }
    
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> cont) noexcept {
        h.promise().continuation = cont;
        return h;
    }

    // Different return types based on ValueType
    void await_resume() requires (std::is_void_v<ValueType>) {
        /*if (h && h.promise().exception) {
            std::rethrow_exception(h.promise().exception);
        }*/
    }
    
    ValueType await_resume() requires (!std::is_void_v<ValueType>) {
        /*if (h && h.promise().exception) {
            std::rethrow_exception(h.promise().exception);
        }*/

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
        h.promise().completed = true;
        if (auto cont = h.promise().continuation)
            cont.resume();
    }
    void await_resume() const noexcept {}
};

// promise implementation used by co; handles return values only
template<typename ValueType>
struct co_promise {
    std::coroutine_handle<> continuation = nullptr;
    std::optional<ValueType> returned_value;
    bool completed = false;

    auto get_return_object() noexcept -> co<ValueType>;
    auto initial_suspend() noexcept { return std::suspend_always{}; }
    auto final_suspend() noexcept -> final_awaiter<co_promise> { return {}; }

    void return_value(ValueType v) noexcept {
        returned_value = std::move(v);
    }

    void unhandled_exception() noexcept { std::terminate(); }
};

// Promise for void-returning coroutines
template<>
struct co_promise<void> {
    std::coroutine_handle<> continuation = nullptr;
    bool completed = false;

    auto get_return_object() noexcept -> co<void>;
    auto initial_suspend() noexcept { return std::suspend_always{}; }
    auto final_suspend() noexcept -> final_awaiter<co_promise> { return {}; }
    void return_void() noexcept {}
    void unhandled_exception() noexcept { std::terminate(); }
};

export template<typename ValueType = void>
class co {
public:
    using promise_type = co_promise<ValueType>;

private:
    std::coroutine_handle<promise_type> _handle = nullptr;

public:
    // constructors / destructor
    co() noexcept = delete;
    explicit co(std::coroutine_handle<promise_type> h) : _handle(h) {
		throw;
        if (!_handle) {
            throw std::runtime_error("Coroutine handle cannot be null");
        }
    }
    co(co&& other) noexcept : _handle(other._handle) { other._handle = nullptr; }
    co& operator=(co&& other) noexcept {
        if (this != &other) {
            if (_handle)
                _handle.destroy();
            _handle = other._handle;
            other._handle = nullptr;
        }
        return *this;
    }
    co(const co&) = delete;
    co& operator=(const co&) = delete;

    ~co() {
        if (_handle)
            _handle.destroy();
    }

    // manual control helpers
    bool done() const noexcept { return !_handle || _handle.promise().completed; }
    void resume() {
        _handle.resume();
    }

    // return value retrieval (only for non-void types)
    template<typename T = ValueType>
    T result() requires (!std::is_void_v<T>) {
        while (!done()) {
            while (_handle.promise().continuation && !_handle.promise().completed) {
                _handle.promise().continuation.resume();
            }

            _handle.resume();
        }

        if (!_handle.promise().returned_value.has_value()) {
            throw std::runtime_error("Coroutine completed without returning a value");
        }
        return *_handle.promise().returned_value;
    }

    // lvalue overload: the caller retains ownership of the handle
    auto operator co_await() & noexcept {
        return awaiter<ValueType, promise_type>{_handle};
    }

    // rvalue overload: transfer ownership to the awaiter so that the
    // temporary object won'y destroy the handle before the coroutine
    // finishes
    auto operator co_await() && noexcept {
        awaiter<ValueType, promise_type> a{_handle};
        _handle = nullptr;
        return a;
    }
};

// out-of-line definitions now that 'co' is complete

template<typename ValueType>
auto co_promise<ValueType>::get_return_object() noexcept -> co<ValueType> {
    return co<ValueType>{std::coroutine_handle<co_promise>::from_promise(*this)};
}

auto co_promise<void>::get_return_object() noexcept -> co<void> {
    return co<void>{std::coroutine_handle<co_promise>::from_promise(*this)};
}
