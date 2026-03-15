export module gs:thread_pool;

import std;
import :channel;

export class thread_pool {
private:
    channel<std::coroutine_handle<>, std::numeric_limits<int>::max()> work_queue;
    std::atomic<bool> shutdown = false;
    std::vector<std::thread> workers;

public:
    explicit thread_pool(std::size_t num_threads = std::thread::hardware_concurrency()) {
        for (std::size_t i = 0; i < num_threads; ++i) {
            workers.emplace_back([this] { worker_loop(); });
        }
    }

    ~thread_pool() {
        shutdown = true;
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

    void enqueue(std::coroutine_handle<> h) {
        work_queue << h;
    }

private:
    void worker_loop() {
        while (!shutdown) {
            if (std::coroutine_handle<> h = work_queue.get(); h) {
                h.resume();
                if (!h.done())
                    enqueue(h);
            }
        }
    }
};
