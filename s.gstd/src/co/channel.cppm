export module gs:channel;
import std;

// Multi-value FIFO channel for synchronizing between threads or coroutines.
// Has a fixed buffer size - blocks when full/empty.
// Has Go-like operators just for the fun of it.
export template<typename T, std::size_t Capacity = 1>
class channel {
    std::mutex m{};
    std::condition_variable cv{};
    std::queue<T> stack{};

public:
    channel& operator<<(T&& val) { set(std::forward<T>(val)); return *this; }
    channel& operator>>(T& out)  { out = get(); return *this; }

    void set(T&& val) {
        {
            std::unique_lock lock(m);
            cv.wait(lock, [this] { return stack.size() < Capacity; }); // block if full
            stack.push(std::forward<T>(val));
        }
        cv.notify_all(); // wake up get() if it was waiting
    }

    T get() {
        std::unique_lock lock(m);
        cv.wait(lock, [this] { return !stack.empty(); }); // block if empty
        T val = std::move(stack.front());
        stack.pop();
        cv.notify_all(); // wake up set() if it was waiting
        return val;
    }
};
