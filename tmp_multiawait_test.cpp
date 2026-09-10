#include <atomic>
#include <condition_variable>
#include <coroutine>
#include <exception>
#include <iostream>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

struct thread_pool {
    struct queue {
        std::vector<std::coroutine_handle<>> q; std::mutex m; std::condition_variable cv; bool closed = false;
        void push(std::coroutine_handle<> h) { std::lock_guard lock(m); q.push_back(h); cv.notify_one(); }
        std::coroutine_handle<> get() { std::unique_lock lock(m); cv.wait(lock, [&]{ return closed || !q.empty(); }); if (q.empty()) return {}; std::coroutine_handle<> h = q.front(); q.erase(q.begin()); return h; }
        void close() { std::lock_guard lock(m); closed = true; cv.notify_all(); }
    } work;
    std::vector<std::jthread> workers;
    thread_pool() { for (int i = 0; i < 2; ++i) workers.emplace_back([this] { while (auto h = work.get()) h.resume(); }); }
    ~thread_pool() { work.close(); }
    static thread_pool& instance() { static thread_pool pool; return pool; }
    template<typename PromiseType> void enqueue(std::coroutine_handle<PromiseType> h) { work.push(h); }
};

template<typename T> class task; template<typename T, typename PromiseType> struct awaiter { std::coroutine_handle<PromiseType> h; bool await_ready() const noexcept { return h.promise().suspended.test() || h.promise().done.test(); } void await_suspend(std::coroutine_handle<> current) noexcept { auto& promise = h.promise(); if (promise.done.test(std::memory_order_acquire)) { thread_pool::instance().enqueue(current); return; } promise.add_continuation(current); } auto await_resume() -> T { auto& p = h.promise(); p.suspended.wait(false); if (p.exception) std::rethrow_exception(p.exception); T v = p.value; if (!p.done.test()) { p.suspended.clear(); p.suspended.notify_one(); } return v; } void await_resume() requires std::is_void_v<T> { auto& p = h.promise(); if (p.exception) std::rethrow_exception(p.exception); if (!p.done.test()) { p.suspended.clear(); p.suspended.notify_one(); } }
};

template<typename T> struct task_promise_base { using value_type = T; std::atomic_flag suspended{}; std::atomic_flag done{}; std::exception_ptr exception = nullptr; std::mutex continuation_mutex; std::vector<std::coroutine_handle<>> continuations; auto initial_suspend() noexcept { done.clear(); auto handle = std::coroutine_handle<task_promise_base>::from_promise(*this); thread_pool::instance().enqueue(handle); return std::suspend_always{}; } auto final_suspend() noexcept -> std::suspend_always { set_done(); return {}; } auto get_return_object() noexcept -> task<T>; void unhandled_exception() noexcept { exception = std::current_exception(); set_done(); } void wait_until_suspended() const noexcept { suspended.wait(false); } void wait_until_not_suspended() const noexcept { suspended.wait(true); } void wait_until_done() const noexcept { done.wait(false); } void add_continuation(std::coroutine_handle<> current) noexcept { std::scoped_lock lock(continuation_mutex); continuations.push_back(current); } std::vector<std::coroutine_handle<>> take_continuations() noexcept { std::scoped_lock lock(continuation_mutex); auto result = std::move(continuations); continuations.clear(); return result; } void set_done() noexcept { set_suspended(); done.test_and_set(); done.notify_one(); for (auto cont : take_continuations()) if (cont) cont.resume(); } void set_suspended() noexcept { suspended.test_and_set(); suspended.notify_one(); } };

template<typename T> struct task_promise : task_promise_base<T> { T value{}; auto get_return_object() noexcept -> task<T>; auto yield_value(T v) noexcept { this->wait_until_not_suspended(); value = std::move(v); this->set_suspended(); return std::suspend_never{}; } void return_value(T v) noexcept { value = std::move(v); this->set_done(); } };

template<> struct task_promise<void> : task_promise_base<void> { auto get_return_object() noexcept -> task<void>; void return_void() noexcept { this->set_done(); } };

template<typename T = void> class task { public: using promise_type = task_promise<T>; using value_type = T; private: std::coroutine_handle<promise_type> h = nullptr; public: task() = default; explicit task(std::coroutine_handle<promise_type> ch) : h(ch) {} task(task&& other) : h(other.h) { other.h = nullptr; } task(const task&) = delete; auto operator=(task&&) = delete; bool done() const noexcept { return !h || h.promise().done.test(); } void wait() const { if (!done()) h.promise().wait_until_done(); } template<typename U = T> U result() const requires (!std::is_void_v<U>) { auto& promise = h.promise(); auto& suspended = promise.suspended; suspended.wait(false); U value = promise.value; if (!promise.done.test()) { suspended.clear(); suspended.notify_one(); } return value; } auto operator co_await() & noexcept { return awaiter<T, promise_type>{h}; } auto operator co_await() && noexcept { return awaiter<T, promise_type>{std::exchange(h, nullptr)}; } };

template<typename T> auto task_promise<T>::get_return_object() noexcept -> task<T> { auto handle = std::coroutine_handle<task_promise>::from_promise(*this); return task<T>{handle}; }
auto task_promise<void>::get_return_object() noexcept -> task<void> { auto handle = std::coroutine_handle<task_promise>::from_promise(*this); return task<void>{handle}; }

task<int> dependency() { co_return 131; }
task<int> d;
task<int> dependent1() { int const val = co_await d; co_return val * 2; }
task<int> dependent2() { int const val = co_await d; co_return val * 3; }

int main() { d = dependency(); auto t1 = dependent1(); auto t2 = dependent2(); std::cout << t1.result() << std::endl << t2.result() << std::endl; }
