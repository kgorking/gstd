// Implements coroutine support.
export module gs:co;

import std;

export template<typename ValueType> class co; // forward declaration for use in promise

// promise implementation used by co; handles return values only
template<typename ValueType>
struct co_promise {
    std::coroutine_handle<> continuation = nullptr;
    std::optional<ValueType> returned_value;

    auto get_return_object() noexcept -> co<ValueType>;
    auto initial_suspend() noexcept { return std::suspend_always{}; }
    struct final_awaiter {
        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<co_promise> h) noexcept {
            if (auto cont = h.promise().continuation)
                cont.resume();
        }
        void await_resume() const noexcept {}
    };
    auto final_suspend() noexcept -> final_awaiter { return {}; }

    void return_value(ValueType v) noexcept {
        returned_value = v;
    }
    
    void unhandled_exception() noexcept { std::terminate(); }
};

// Promise for void-returning coroutines
template<>
struct co_promise<void> {
    std::coroutine_handle<> continuation = nullptr;
    auto get_return_object() noexcept -> co<void>;
    auto initial_suspend() noexcept { return std::suspend_always{}; }
    struct final_awaiter {
        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<co_promise> h) noexcept {
            if (auto cont = h.promise().continuation)
                cont.resume();
        }
        void await_resume() const noexcept {}
    };
    auto final_suspend() noexcept -> final_awaiter { return {}; }
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
    co() noexcept = default;
    explicit co(std::coroutine_handle<promise_type> h) noexcept : _handle(h) {}
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
    bool done() const noexcept { return !_handle || _handle.done(); }
    void resume() noexcept { if (_handle) _handle.resume(); }
    co& wait() noexcept { while (!done()) resume(); return *this; }
    

    // return value retrieval (only for non-void types)
    template<typename T = ValueType>
    std::optional<T> result() const noexcept requires (!std::is_void_v<T>) {
        return _handle.promise().returned_value;
    }

    // awaiter support with value handling
    struct awaiter {
        std::coroutine_handle<promise_type> h;

        // if coroutine is done no suspension, otherwise always suspend
        bool await_ready() const noexcept { return !h || h.done(); }
        
        void await_suspend(std::coroutine_handle<> cont) noexcept {
            h.promise().continuation = cont;
            h.resume();
        }
        
        // Different return types based on ValueType
        void await_resume() noexcept requires (std::is_void_v<ValueType>) {}
        
        ValueType await_resume() noexcept requires (!std::is_void_v<ValueType>) {
            // return stored return value, or default-constructed if absent
            if (h.promise().returned_value.has_value())
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

// out-of-line definitions now that 'co' is complete

template<typename ValueType>
auto co_promise<ValueType>::get_return_object() noexcept -> co<ValueType> {
    return co<ValueType>{std::coroutine_handle<co_promise>::from_promise(*this)};
}

auto co_promise<void>::get_return_object() noexcept -> co<void> {
    return co<>{std::coroutine_handle<co_promise>::from_promise(*this)};
}
