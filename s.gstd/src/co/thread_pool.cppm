export module gs:thread_pool;

import std;
import :channel;

export class thread_pool {
private:
    channel<std::coroutine_handle<>> work_queue;
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

    // Delete copy operations
    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;

    // Get the global thread pool instance
    static thread_pool& instance() {
        static thread_pool pool;
        return pool;
    }

    // Enqueue a work item to be executed by a thread pool worker
    void enqueue(std::coroutine_handle<> h) {
        work_queue << h;
    }

private:
    void worker_loop() {
        std::coroutine_handle<> h = nullptr;
        while (true) {
            work_queue >> h;
            if (shutdown || !h) {
                break;
            }

            h.resume();
            if (!h.done()) {
                // If the coroutine is not done, re-enqueue it to be resumed later
                enqueue(h);
            }
        }
    }
};
