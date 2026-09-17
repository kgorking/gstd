export module gs:thread_pool;

import std;
import :types;
import :channel;
import :task;

export class thread_pool {
private:
	std::array<std::atomic<std::coroutine_handle<>>*, 64> worker_handles{};

	channel<std::coroutine_handle<>> work_queue;
	channel<std::coroutine_handle<>> io_work_queue;
	std::vector<std::jthread> workers;
	std::vector<std::jthread> io_workers;

	thread_local inline static bool is_worker_thread = false;
	thread_local inline static bool is_io_thread = false;

	struct threaded_waiter {
		bool await_ready() const noexcept { return is_worker_thread; } // Don't reschedule if already on a worker thread.
		template <typename T>
		void await_suspend(std::coroutine_handle<task_promise<T>> h) noexcept {
			static_assert(std::is_void_v<T>, "Can only be called from a task<void>. Use a channel<> to pass values between threads.");
			thread_pool::instance().enqueue(h);
		}
		void await_resume() noexcept {}
	};

	struct io_waiter {
		bool await_ready() const noexcept { return is_io_thread; } // Don't reschedule if already on an io thread.
		template <typename T>
		void await_suspend(std::coroutine_handle<task_promise<T>> h) noexcept {
			static_assert(std::is_void_v<T>, "Can only be called from a task<void>. Use a channel<> to pass values between threads.");
			thread_pool::instance().enqueue_io(h);
		}
		void await_resume() noexcept {}
	};

public:
	explicit thread_pool(int64 num_threads = std::jthread::hardware_concurrency() - 1) {
		if (num_threads <= 0)
			throw std::runtime_error("Bad thread count");

		workers.reserve(num_threads);
		io_workers.reserve(num_threads);
		for (uint64 i = 0; i < (uint64)num_threads; ++i) {
			workers.emplace_back([this, i](std::stop_token stoken) { worker_loop(stoken, i); });
			io_workers.emplace_back([this](std::stop_token stoken) { io_worker_loop(stoken); });
		}
	}

	~thread_pool() {
		work_queue.close();
		io_work_queue.close();
		std::ranges::for_each(workers, &std::jthread::request_stop);
		std::ranges::for_each(io_workers, &std::jthread::request_stop);
		for (auto& w : worker_handles) {
			if (w) {
				w->store(std::noop_coroutine());
				w->notify_one();
			}
		}
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
	void enqueue(std::coroutine_handle<> h) {
		for (uint64 i = 0; i < worker_handles.size(); i++) {
			std::coroutine_handle<> expected = nullptr;
			if (worker_handles[i] && worker_handles[i]->compare_exchange_weak(expected, h)) {
				worker_handles[i]->notify_one();
				return;
			}
		}

		std::println("enqued in slot queue");
		work_queue << h;
	}
	void enqueue_io(std::coroutine_handle<> h) { io_work_queue << h; }

	void worker_loop(std::stop_token stoken, uint64 i) {
		is_worker_thread = true;
		std::atomic<std::coroutine_handle<>> shared_h;
		worker_handles[i] = &shared_h;

		while (!stoken.stop_requested()) {
			std::coroutine_handle<> h = nullptr;
			if (shared_h != nullptr) {
				// Do work from the local slot
				h = shared_h;
				shared_h = nullptr;
				h.resume();
			}
			else if (work_queue.try_get(h)) {
				// Do work from the global list
				h.resume();
			}
			else {
				// Wait for work being ready
				shared_h.wait(nullptr);
			}
		}

		worker_handles[i] = nullptr;
		//while (!stoken.stop_requested() && (h = work_queue.get()))
		//	h.resume();
	}

	void io_worker_loop(std::stop_token stoken) {
		is_io_thread = true;
		std::coroutine_handle<> h = nullptr;
		while (!stoken.stop_requested() && (h = io_work_queue.get()))
			h.resume();
	}
};
