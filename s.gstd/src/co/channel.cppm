export module gs:channel;
import std;

// Multi-value FIFO channel for synchronizing between threads or coroutines.
// Has a fixed buffer size - blocks when full/empty.
// Has Go-like operators just for the fun of it.
export template<typename T, int Capacity = 0>
    requires (Capacity >= 0)
class channel {
    using data_type = std::conditional_t<Capacity == 0, std::optional<T>, std::queue<T>>;
    std::mutex m{};
    std::condition_variable cv{};
    data_type data{};
    bool stopped = false;

public:
    channel& operator<<(T val) { set(std::move(val)); return *this; }
    channel& operator>>(T& out)  { out = get(); return *this; }

    void set(T val) {
        {
            if constexpr (Capacity > 0) {
                std::unique_lock lock(m);
                cv.wait(lock, [this] { return data.size() < Capacity || stopped; });
                if (stopped) return; // abort if channel was stopped
                data.push(std::move(val));
            } else {
                std::unique_lock lock(m);
                cv.wait(lock, [this] { return !data.has_value() || stopped; });
                if (stopped) return; // abort if channel was stopped
                data = std::move(val);
            }
        }
        cv.notify_all(); // wake up get() if it was waiting
    }

    T get() {
        std::unique_lock lock(m);
        T val;
        if constexpr (Capacity > 0) {
            cv.wait(lock, [this] { return stopped || !data.empty(); });
            if (stopped && data.empty()) return T{}; // return default if channel was stopped and is empty
            val = std::move(data.front());
            data.pop();
        } else {
            cv.wait(lock, [this] { return stopped || data.has_value(); });
            if (stopped && !data.has_value()) return T{}; // return default if channel was stopped and is empty
            val = std::move(*data);
            data.reset();
        }
        cv.notify_all(); // wake up set() if it was waiting
        return val;
    }

    void close() {
        {
            std::unique_lock lock(m);
            stopped = true;
        }
        cv.notify_all(); // wake up all waiting threads
    }
};
