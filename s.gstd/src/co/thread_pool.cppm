export module gs:thread_pool;

import std;

export class thread_pool {
private:
    // Base class for type erasure of work items
    struct work_base {
        virtual ~work_base() = default;
        virtual void execute() = 0;
    };

    // Template work item
    template<typename Func>
    struct work_item : work_base {
        Func fn;
        explicit work_item(Func&& f) noexcept : fn(std::forward<Func>(f)) {}
        void execute() override { fn(); }
    };

    std::vector<std::thread> workers;
    std::queue<std::unique_ptr<work_base>> work_queue;
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
    template<typename Func>
    void enqueue(Func&& fn) {
        {
            std::lock_guard lock(queue_mutex);
            work_queue.push(std::make_unique<work_item<Func>>(std::forward<Func>(fn)));
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
            std::unique_ptr<work_base> item;
            {
                std::unique_lock lock(queue_mutex);
                cv.wait(lock, [this] { return !work_queue.empty() || shutdown; });

                if (shutdown && work_queue.empty())
                    break;

                if (!work_queue.empty()) {
                    item = std::move(work_queue.front());
                    work_queue.pop();
                } else {
                    continue;
                }
            }
            item->execute();
        }
    }
};
