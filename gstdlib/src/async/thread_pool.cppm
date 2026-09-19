export module gs:thread_pool;

import std;
import :types;
import :channel;
import :task;

export class thread_pool {
private:
	using shared_handle = std::atomic<std::coroutine_handle<>>*;
	static constexpr int Cacheline = 64;
	static constexpr int N = 24;
	static constexpr int N_bytes = N * sizeof(shared_handle);
	static constexpr int N_per_cacheline = Cacheline / sizeof(shared_handle);
	static constexpr int N_cachelines = N_bytes / Cacheline;

	std::array<shared_handle, N> worker_handles{};
	std::array<bool, N_cachelines> worker_dirty{};

	channel<std::coroutine_handle<>> work_queue;
	channel<std::coroutine_handle<>> io_work_queue;
	std::vector<std::jthread> workers;
	std::vector<std::jthread> io_workers;

	std::latch thread_initializition;

	thread_local inline static bool is_worker_thread = false;
	thread_local inline static bool is_io_thread = false;

	struct threaded_waiter {
		bool await_ready() const noexcept { return is_worker_thread; } // Don't reschedule if already on a worker thread.
		template <typename T, bool B>
		void await_suspend(std::coroutine_handle<task_promise<T,B>> h) noexcept {
			static_assert(std::is_void_v<T>, "Can only be called from a task<void>. Use a channel<> to pass values between threads.");
			thread_pool::instance().enqueue(h);
		}
		void await_resume() noexcept {}
	};
	struct io_waiter {
		bool await_ready() const noexcept { return is_io_thread; } // Don't reschedule if already on an io thread.
		template <typename T, bool B>
		void await_suspend(std::coroutine_handle<task_promise<T,B>> h) noexcept {
			static_assert(std::is_void_v<T>, "Can only be called from a task<void>. Use a channel<> to pass values between threads.");
			thread_pool::instance().enqueue_io(h);
		}
		void await_resume() noexcept {}
	};

public:
	explicit thread_pool(int64 num_threads = std::jthread::hardware_concurrency() - 1)
		: thread_initializition(num_threads) {
		if (num_threads <= 0)
			throw std::runtime_error("Bad thread count");

		workers.reserve(num_threads);
		io_workers.reserve(num_threads);
		for (uint64 i = 0; i < (uint64)num_threads; ++i) {
			workers.emplace_back([this, i](std::stop_token stoken) { worker_loop(stoken, i); });
			io_workers.emplace_back([this](std::stop_token stoken) { io_worker_loop(stoken); });
		}

		// Wait for worker threads to finish their initializations
		thread_initializition.wait();
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
		for (uint64 j = 0; j < worker_dirty.size(); j++) {
			// If there has been no changes in the cacheline of workers, just skip them all
			if (!worker_dirty[j])
				continue;

			uint64 const first = j * N_per_cacheline;
			uint64 const last = (1 + j) * N_per_cacheline;

			for (uint64 i = first; i < last; i++) {
				std::coroutine_handle<> expected = nullptr;
				if (worker_handles[i]->compare_exchange_weak(expected, h)) {
					//std::println("enqued in slot {}", i);
					worker_handles[i]->notify_one();
					return;
				}
			}

			worker_dirty[j] = false;
		}

		//std::println("enqued in slot queue");
		work_queue << h;
	}
	void enqueue_io(std::coroutine_handle<> h) { io_work_queue << h; }

	void worker_loop(std::stop_token stoken, uint64 const i) {
		is_worker_thread = true;

		// Receive work here from main thread
		std::atomic<std::coroutine_handle<>> shared_h;
		worker_handles[i] = &shared_h;

		// Notify main thread with this when work is completed
		bool *cacheline_dirty = &worker_dirty[i / N_per_cacheline];
		*cacheline_dirty = true;

		// Signal that this threads initialization is done
		thread_initializition.count_down();

		// Worker loop
		while (!stoken.stop_requested()) {
			std::coroutine_handle<> h = shared_h.exchange(nullptr);
			if (h && !h.done()) {
				// Do work from the local slot
				*cacheline_dirty = true;
				h.resume();
			}
			else if (work_queue.try_get(h) && !h.done()) {
				// Do work from the global list
				h.resume();
			}
			else {
				// Wait for work being ready
				shared_h.wait(nullptr);
			}
		}

		worker_handles[i] = nullptr;
	}

	void io_worker_loop(std::stop_token stoken) {
		is_io_thread = true;
		std::coroutine_handle<> h = nullptr;
		while (!stoken.stop_requested() && (h = io_work_queue.get()))
			h.resume();
	}
};
