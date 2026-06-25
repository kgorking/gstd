// Implements coroutine-based generator sequences.
export module gs:sequence;

import std;
import :concepts;

template <typename MapFn, typename ...Ts>
    requires std::invocable<MapFn, Ts...>
using map_r = std::remove_cvref_t<std::invoke_result_t<MapFn, Ts...>>;

export template<typename T>
class sequence {
public:
    struct promise_type {
        T current{};
        std::function<bool()> pending_fn;

        auto get_return_object() noexcept -> sequence {
            return sequence{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        auto initial_suspend() noexcept { return std::suspend_always{}; }
        auto final_suspend() noexcept { return std::suspend_always{}; }

        auto yield_value(T&& value) noexcept {
            current = std::forward<T>(value);
            return std::suspend_always{};
        }

        auto yield_value(T const& value) noexcept {
            current = value;
            return std::suspend_always{};
        }

        template<std::ranges::range R>
            requires std::convertible_to<std::ranges::range_value_t<R>, T>
        auto yield_value(std::ranges::elements_of<R> elem) noexcept {
            auto it = std::ranges::begin(elem.range);
            auto end = std::ranges::end(elem.range);
            
            if (it != end) {
                current = *it;
                ++it;
                pending_fn = [this, it, end]() mutable {
                    if (it != end) {
                        current = *it;
                        ++it;
                        return true;
                    }
                    return false;
                };
            }
            
            return std::suspend_always{};
        }

        void return_void() noexcept {}
        void unhandled_exception() noexcept { std::terminate(); }
    };

    struct iterator {
        using difference_type = std::ptrdiff_t;
        using value_type = T;

        std::coroutine_handle<promise_type> _handle = nullptr;
        bool _exhausted = true;

        iterator() = default;
        explicit iterator(std::coroutine_handle<promise_type> h)
            : _handle(h), _exhausted(false) {
            if (_handle) {
                _handle.resume();
                if (_handle.done())
                    _exhausted = true;
            }
        }

        T& operator*() const { return _handle.promise().current; }

        iterator& operator++() {
            if (_handle) {
                promise_type& promise = _handle.promise();
                
                // Check if there are pending elements from elements_of
                if (promise.pending_fn && promise.pending_fn()) {
                    // pending_fn already updated current
                    return *this;
                }
                
                // Clear pending function
                promise.pending_fn = nullptr;
                
                _handle.resume();
                if (_handle.done())
                    _exhausted = true;
            }
            return *this;
        }
        void operator++(int) { ++*this; }

        bool operator==(std::default_sentinel_t) const { return _handle.done(); }
    };
    static_assert(std::input_or_output_iterator<iterator>);

private:
    std::coroutine_handle<promise_type> _handle;

public:
    explicit sequence(std::coroutine_handle<promise_type> h) noexcept : _handle(h) {}

    sequence() noexcept : _handle(nullptr) {}
    sequence(sequence&& other) noexcept : _handle(std::exchange(other._handle, nullptr)) {}
    sequence& operator=(sequence&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        if (_handle)
            _handle.destroy();
        _handle = other._handle;
        other._handle = nullptr;
        return *this;
    }
    sequence(const sequence&) = delete;
    sequence& operator=(const sequence&) = delete;

    ~sequence() {
        if (_handle)
            _handle.destroy();
        _handle = nullptr;
    }

    bool next() {
        if (!_handle || _handle.done())
            return false;
        _handle.resume();
        return !_handle.done();
    }

    bool done() const {
         return !_handle || _handle.done();
    }

    T value() const noexcept {
        return _handle.promise().current;
    }

    iterator begin() const noexcept {
        return iterator(_handle);
    }

    std::default_sentinel_t end() const noexcept { return {}; }

	sequence<T> concat(this sequence<T> self, sequence<T> other) {
		while (self.next()) {
			co_yield self.value();
		}
		while (other.next()) {
			co_yield other.value();
		}
	}

	[[nodiscard]]
    sequence<T> where(this Seq auto self, Predicate<T> auto&& pred) {
        while (self.next()) {
            T v = self.value();
            if (pred(v)) co_yield v;
        }
    }

    template<std::invocable<T> Fn>
    [[nodiscard]]
    auto map(this Seq auto self, Fn&& fn) -> sequence<map_r<Fn, T>> {
        while (self.next()) {
            co_yield fn(self.value());
        }
    }

    [[nodiscard]]
    auto take(this Seq auto self, std::signed_integral auto count) -> sequence<T> {
        while (count-- > 0 && self.next()) {
            co_yield self.value();
        }
    }

    [[nodiscard]]
    auto drop(this Seq auto self, std::signed_integral auto count) -> sequence<T> {
        while (count-- > 0 && self.next()) {
            // just drop the value
        }
        while (self.next()) {
            co_yield self.value();
        }
    }

    template<typename U>
    [[nodiscard]]
    auto zip(this Seq auto self, sequence<U> other) -> sequence<std::tuple<T, U>> {
        while (self.next() && other.next()) {
            co_yield std::tuple<T, U>(self.value(), other.value());
        }
    }

    template<typename U, std::invocable<T, U> Fn>
    [[nodiscard]]
    auto zipmap(this Seq auto self, sequence<U> other, Fn&& fn) -> sequence<map_r<Fn, T, U>> {
        while (self.next() && other.next()) {
            co_yield fn(self.value(), other.value());
        }
    }

    void for_each(std::invocable<T> auto&& fn) {
        while (next()) {
            fn(value());
        }
    }

    template<typename Comp = std::less<>>
    [[nodiscard]]
    auto merge(this sequence<T> self, sequence<T> other,  Comp comp = std::less{}) -> sequence<T> {
        bool done = !(self.next() && other.next());
        while (!done) {
            if (comp(self.value(), other.value())) {
                co_yield self.value();
                done = !self.next();
            } else {
                co_yield other.value();
                done = !other.next();
            }
        }

        while (self.next()) {
            co_yield self.value();
        }
        while (other.next()) {
            co_yield other.value();
        }
    }
};
static_assert(std::ranges::range<sequence<int>>);
