export module gs:thread_pool;

import std;
import :types;
import :channel;
import :task;

export class thread_pool {
private:
	channel<std::coroutine_handle<>> work_queue;
	channel<std::coroutine_handle<>> io_work_queue;
	std::vector<std::jthread> workers;
	std::vector<std::jthread> io_workers;

	// TODO
	thread_local inline static bool is_io_thread = false;
	thread_local inline static bool is_worker_thread = false;

	struct threaded_waiter {
		bool await_ready() const noexcept { return false; }
#ifdef __cpp_deleted_function
		void await_suspend(auto) = delete ("Can only be called from a task<void>. Use a channel<> to pass values between threads.");
		void await_suspend(std::coroutine_handle<task_promise<void>> h) noexcept {
			thread_pool::instance().enqueue(h);
		}
#else
		template <typename T>
		void await_suspend(std::coroutine_handle<task_promise<T>> h) noexcept {
			static_assert(std::is_void_v<T>, "Can only be called from a task<void>. Use a channel<> to pass values between threads.");
			thread_pool::instance().enqueue(h);
		}
#endif
		void await_resume() noexcept {}
	};

	struct io_waiter {
		bool await_ready() const noexcept { return false; }
		template <typename T>
		void await_suspend(std::coroutine_handle<task_promise<T>> h) noexcept {
			static_assert(std::is_void_v<T>, "Can only be called from a task<void>. Use a channel<> to pass values between threads.");
			thread_pool::instance().enqueue_io(h);
		}
		void await_resume() noexcept {}
	};

public:
	explicit thread_pool(int64 num_threads = std::jthread::hardware_concurrency() - 1) {
		workers.reserve(num_threads);
		io_workers.reserve(num_threads);
		for (int64 i = 0; i < num_threads; ++i) {
			workers.emplace_back([this](std::stop_token stoken) { worker_loop(stoken); });
			io_workers.emplace_back([this](std::stop_token stoken) { io_worker_loop(stoken); });
		}
	}

	~thread_pool() {
		work_queue.close();
		io_work_queue.close();
		std::ranges::for_each(workers, &std::jthread::request_stop);
		std::ranges::for_each(io_workers, &std::jthread::request_stop);
	}

	thread_pool(const thread_pool&) = delete;
	thread_pool& operator=(const thread_pool&) = delete;

	static thread_pool& instance() {
		static thread_pool pool;
		return pool;
	}

	[[nodiscard]]
	static auto switch_to_thread() {
		return threaded_waiter{};
	}

	[[nodiscard]]
	static auto switch_to_io() {
		return io_waiter{};
	}

private:
	void enqueue(std::coroutine_handle<> h) { work_queue << h; }
	void enqueue_io(std::coroutine_handle<> h) { io_work_queue << h; }

	void worker_loop(std::stop_token stoken) {
		is_worker_thread = true;
		std::coroutine_handle<> h = nullptr;
		while (!stoken.stop_requested() && (h = work_queue.get()))
			h.resume();
	}

	void io_worker_loop(std::stop_token stoken) {
		is_io_thread = true;
		std::coroutine_handle<> h = nullptr;
		while (!stoken.stop_requested() && (h = io_work_queue.get()))
			h.resume();
	}
};
