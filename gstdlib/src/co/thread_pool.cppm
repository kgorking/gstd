export module gs:thread_pool;

import std;
import :channel;

export class thread_pool {
private:
    channel<std::coroutine_handle<>> work_queue;
    channel<std::coroutine_handle<>> io_work_queue;
    std::vector<std::jthread> workers;
    std::vector<std::jthread> io_workers;

public:
    explicit thread_pool(std::size_t num_threads = std::jthread::hardware_concurrency() - 1) {
        for (std::size_t i = 0; i < num_threads; ++i) {
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

	template<typename PromiseType>
    void enqueue(std::coroutine_handle<PromiseType> h) {
		work_queue << h;
    }

	template<typename PromiseType>
    void enqueue_io(std::coroutine_handle<PromiseType> h) {
		io_work_queue << h;
    }

private:
    void worker_loop() {
		while (std::coroutine_handle<> h = work_queue.get())
			h.resume();
    }

    void io_worker_loop() {
		while (std::coroutine_handle<> h = io_work_queue.get())
			h.resume();
    }
};
