export module gs:signal;
import std;

// Single-value signal for synchronizing between threads or coroutines
export template<typename T>
class signal {
    std::mutex m{};
    std::condition_variable cv{};
    std::optional<T> value{};

public:
    void set(T val) {
        {
            std::lock_guard lock(m);
            value = std::move(val);
        }
        cv.notify_all();
    }

    T get() {
        std::unique_lock lock(m);
        cv.wait(lock, [this] { return value.has_value(); });
        return *std::exchange(value, {});
    }
};
