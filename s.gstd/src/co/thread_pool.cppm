export module gs:thread_pool;

import std;
import :channel;

export class thread_pool {
private:
    channel<std::coroutine_handle<>, 64> work_queue;
    std::vector<std::thread> workers;

public:
    explicit thread_pool(std::size_t num_threads = std::thread::hardware_concurrency() - 1) {
        for (std::size_t i = 0; i < num_threads; ++i) {
            workers.emplace_back([this] { worker_loop(); });
        }
    }

    ~thread_pool() {
        work_queue.close();
        for (auto& w : workers) {
            if (w.joinable())
                w.join();
        }
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

private:
    void worker_loop() {
		while (std::coroutine_handle<> h = work_queue.get())
			h.resume();
    }
};
