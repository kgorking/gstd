#include <coroutine>
#include <future>
#include <iostream>
#include <thread>

// Custom awaitable type that wraps std::future
template<typename T>
struct Task {
    struct promise_type {
        std::promise<T> prom;

        Task get_return_object() {
            return Task{ prom.get_future() };
        }
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }

        void return_value(T value) {
            prom.set_value(value);
        }
        void unhandled_exception() {
            try {
                throw;
            } catch (...) {
                prom.set_exception(std::current_exception());
            }
        }
    };

    std::future<T> fut;
    T get() { return fut.get(); }
};

// Coroutine function
Task<int> compute_sum(int a, int b) {
    // Simulate async work
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    co_return a + b;
}

int main() {
    try {
        auto task = compute_sum(10, 32); // Start coroutine
        int result = task.get();         // Wait and get result
        std::cout << "Result from coroutine: " << result << "\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}
