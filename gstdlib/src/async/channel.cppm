export module gs:channel;
import std;

// Multi-value FIFO channel for synchronizing between threads or coroutines.
// Has a fixed buffer size - blocks when writing to a full channel or reading from an empty channel.
// Has Go-like operators just for the fun of it.
export template<typename T>
class channel {
    using data_type = std::queue<T>;
	mutable std::mutex m{};
    mutable std::condition_variable cv_not_empty{};
    data_type data{};
    bool stopped = false;

public:
	~channel() {
		close();
	}

	channel& operator<<(T val) { set(std::move(val)); return *this; }
    channel& operator>>(T& out)  { out = get(); return *this; }
	T operator*() { return get(); }

	bool is_stopped() const {
		std::unique_lock lock(m);
		return stopped;
	}

    void set(T val) {
        std::unique_lock lock(m);
		if (stopped)
			throw std::runtime_error("channel is closed");

        data.push(std::move(val));
        cv_not_empty.notify_one();
    }

    T get() {
	    std::unique_lock lock(m);
		cv_not_empty.wait(lock, [this] { return stopped || !data.empty(); });
        if (stopped && data.empty()) return T{};
        T val = std::move(data.front());
        data.pop();

        return val;
    }

	bool try_get(T& out) {
		std::unique_lock lock(m);
		if (stopped || data.empty())
			return false;

		out = std::move(data.front());
		data.pop();
		return true;
	}

    void close() {
        {
            std::unique_lock lock(m);
			if (stopped)
				return;
            stopped = true;
        }
        cv_not_empty.notify_all();
    }
};
