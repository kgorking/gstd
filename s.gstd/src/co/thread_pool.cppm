export module gs:thread_pool;

import std;

export class thread_pool {
private:
    std::vector<std::thread> workers;
    std::queue<std::coroutine_handle<>> work_queue;
    std::mutex queue_mutex;
    std::condition_variable cv;
    std::atomic<bool> shutdown = false;

public:
    explicit thread_pool(std::size_t num_threads = std::thread::hardware_concurrency()) {
        for (std::size_t i = 0; i < num_threads; ++i) {
            workers.emplace_back([this] { worker_loop(); });
        }
    }

    ~thread_pool() {
        shutdown = true;
        cv.notify_all();
        for (auto& w : workers) {
            if (w.joinable())
                w.join();
        }
    }

    // Delete copy operations
    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;

    // Enqueue a work item to be executed by a thread pool worker
    void enqueue(std::coroutine_handle<> h) {
        {
            std::lock_guard lock(queue_mutex);
            work_queue.push(h);
        }
        cv.notify_one();
    }

    // Get the global thread pool instance
    static thread_pool& instance() {
        static thread_pool pool;
        return pool;
    }

private:
    void worker_loop() {
        while (true) {
            std::coroutine_handle<> item;
            {
                std::unique_lock lock(queue_mutex);
                cv.wait(lock, [this] { return !work_queue.empty() || shutdown; });

                if (shutdown && work_queue.empty())
                    break;

                if (!work_queue.empty()) {
                    item = work_queue.front();
                    work_queue.pop();
                } else {
                    continue;
                }
            }

            // Run coroutine to completion
            while (!item.done()) {
                item.resume();
            }
        }
    }
};
