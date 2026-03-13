export module gs:signal;
import std;

// Single-value signal for synchronizing between threads or coroutines
export template<typename T>
class signal {
    mutable std::mutex mutex;
    std::condition_variable cv;
    T value{};
    bool value_ready = false;

public:
    void set(T val) {
        {
            std::lock_guard lock(mutex);
            value = std::move(val);
            value_ready = true;
        }
        cv.notify_one(); // wake up waiting consumer
    }

    T get() {
        std::unique_lock lock(mutex);
        cv.wait(lock, [this] { return value_ready; });
        value_ready = false;
        return std::move(value);
    }
};
