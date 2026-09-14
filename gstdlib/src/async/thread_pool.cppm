export module gs:thread_pool;

import std;
import :types;
import :channel;

export class thread_pool {
private:
    channel<std::coroutine_handle<>, 256> work_queue;
    channel<std::coroutine_handle<>, std::numeric_limits<int>::max()> io_work_queue;
    std::vector<std::jthread> workers;
    std::vector<std::jthread> io_workers;
	thread_local inline static bool is_io_thread = false;
	thread_local inline static bool is_worker_thread = false;

public:
    explicit thread_pool(int64 num_threads = std::jthread::hardware_concurrency() - 1) {
		workers.reserve(num_threads);
		io_workers.reserve(num_threads);
        for (int64 i = 0; i < num_threads; ++i) {
            workers.emplace_back([this] { worker_loop(); });
            io_workers.emplace_back([this] { io_worker_loop(); });
        }
    }

    ~thread_pool() {
        work_queue.close();
        io_work_queue.close();
    }

    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;

    static thread_pool& instance() {
        static thread_pool pool;
        return pool;
    }

	[[nodiscard]]
	static auto switch_to_thread() {
		struct waiter {
			bool await_ready() const noexcept { return is_worker_thread; }
			bool await_suspend(std::coroutine_handle<> h) noexcept {
				if (is_worker_thread)
					return false;
				thread_pool::instance().enqueue(h);
				return true;
			}
			void await_resume() noexcept {}
		};
		return waiter{};
	}

	[[nodiscard]]
	static auto switch_to_io() {
		struct waiter {
			bool await_ready() const noexcept { return is_io_thread; }
			bool await_suspend(std::coroutine_handle<> h) noexcept {
				if (is_io_thread)
					return false;
				thread_pool::instance().enqueue_io(h);
				return true;
			}
			void await_resume() noexcept {}
		};
		return waiter{};
	}

private:
    void enqueue(std::coroutine_handle<> h) { work_queue << h; }
    void enqueue_io(std::coroutine_handle<> h) { io_work_queue << h; }

    void worker_loop() {
		is_worker_thread = true;
		while (std::coroutine_handle<> h = work_queue.get())
			h.resume();
    }

    void io_worker_loop() {
		is_io_thread = true;
		while (std::coroutine_handle<> h = io_work_queue.get())
			h.resume();
    }
};
