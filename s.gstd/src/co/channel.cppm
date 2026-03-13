export module gs:channel;
import std;

// Multi-value FIFO channel for synchronizing between threads or coroutines.
// Has Go-like operators just for the fun of it.
export template<typename T>
class channel {
    std::mutex m{};
    std::condition_variable cv{};
    std::queue<T> stack{};

public:
    channel& operator<<(T&& val) { set(std::forward<T>(val)); return *this; }
    channel& operator>>(T& out)  { out = get(); return *this; }

    void set(T&& val) {
        {
            std::lock_guard lock(m);
            stack.push(std::forward<T>(val));
        }
        cv.notify_all();
    }

    T get() {
        std::unique_lock lock(m);
        if (stack.empty())
            cv.wait(lock, [this] { return !stack.empty(); });
        T val = std::move(stack.front());
        stack.pop();
        return val;
    }
};
