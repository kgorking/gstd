export module gs:channel;
import std;

// Multi-value FIFO channel for synchronizing between threads or coroutines.
// Has a fixed buffer size - blocks when writing to a full channel or reading from an empty channel.
// Has Go-like operators just for the fun of it.
export template<typename T, std::signed_integral auto Capacity = 0>
    requires (Capacity >= 0)
class channel {
    using data_type = std::conditional_t<Capacity == 0, std::optional<T>, std::queue<T>>;
    data_type data{};
    // Two CVs: setters wait for space, getters wait for data.
    // A single CV with notify_one() here loses wakeups: set() must wake a
    // getter but may wake another setter instead (and vice versa), leaving
    // the thread that could make progress asleep -> occasional hang.
    mutable std::condition_variable cv_not_full{};
    mutable std::condition_variable cv_not_empty{};
	mutable std::mutex m{};
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

	bool is_set() const {
		std::unique_lock lock(m);
		if constexpr (Capacity > 0) {
			return !data.empty();
		}
		else {
			return data.has_value();
		}
	}

    void set(T val) {
        if constexpr (Capacity > 0) {
            std::unique_lock lock(m);
            cv_not_full.wait(lock, [this] { return stopped || data.size() < Capacity; });
            if (stopped) return;
            data.push(std::move(val));
        } else {
            std::unique_lock lock(m);
            cv_not_full.wait(lock, [this] { return stopped || !data.has_value(); });
            if (stopped) return;
            data = std::move(val);
        }

        cv_not_empty.notify_one();
    }

    T get() {
		T val;
        if constexpr (Capacity > 0) {
	        std::unique_lock lock(m);
            cv_not_empty.wait(lock, [this] { return stopped || !data.empty(); });
            if (stopped && data.empty()) return T{};
            val = std::move(data.front());
            data.pop();
        } else {
	        std::unique_lock lock(m);
            cv_not_empty.wait(lock, [this] { return stopped || data.has_value(); });
            if (stopped && !data.has_value()) return T{};
            val = std::move(*data);
            data.reset();
        }

        cv_not_full.notify_one();
        return val;
    }

    void close() {
        {
            std::unique_lock lock(m);
			if (stopped)
				return;
            stopped = true;
        }
        cv_not_full.notify_all();
        cv_not_empty.notify_all();
    }
};

template<>
class channel<void> {
	mutable std::condition_variable cv_not_full{};
	mutable std::condition_variable cv_not_empty{};
	mutable std::mutex m{};
	std::exception_ptr exception_ptr_;
	bool set_flag = false;
	bool stopped = false;

public:
	void set() {
		{
			std::unique_lock lock(m);
			cv_not_full.wait(lock, [this] { return stopped || !set_flag; });
			if (stopped) return; // abort if channel was stopped
			set_flag = true;
		}
		cv_not_empty.notify_one(); // wake up get() waiting for data
	}

	bool is_stopped() const {
		std::lock_guard lock(m);
		return stopped;
	}

	bool is_set() const {
		std::lock_guard lock(m);
		return set_flag;
	}

	void get() {
		{
			std::unique_lock lock(m);
			cv_not_empty.wait(lock, [this] { return stopped || set_flag; });
			if (stopped) return;
			set_flag = false;
		}
		cv_not_full.notify_one(); // wake up set() waiting for space
	}

	void close() {
		{
			std::unique_lock lock(m);
			if (stopped)
				return;
			set_flag = false;
			stopped = true;
		}
		cv_not_full.notify_all(); // wake up all waiting threads
		cv_not_empty.notify_all();
	}
};
