module;
#include <windows.h>
export module gs:io_scheduler;

import std;

export class io_scheduler {
private:
    struct pending_operation {
        std::coroutine_handle<> continuation;
        std::int64_t result = 0;
        bool is_error = false;
    };

    HANDLE iocp_handle;
    std::thread dispatcher_thread;
    std::atomic<bool> running = false;
    std::unordered_map<OVERLAPPED*, pending_operation> pending_ops;
    std::mutex pending_ops_mutex;

public:
    io_scheduler() {
        // Create the I/O Completion Port
        iocp_handle = CreateIoCompletionPort(
            INVALID_HANDLE_VALUE,
            nullptr,
            0,
            0  // Let system choose concurrency level
        );
        
        if (!iocp_handle) {
            throw std::system_error(std::make_error_code(std::errc::resource_unavailable_try_again));
        }

        running = true;
        dispatcher_thread = std::thread([this] { dispatch_loop(); });
    }

    ~io_scheduler() {
        running = false;
        if (dispatcher_thread.joinable()) {
            dispatcher_thread.join();
        }
        if (iocp_handle) {
            CloseHandle(iocp_handle);
        }
    }

    // Delete copy operations
    io_scheduler(const io_scheduler&) = delete;
    io_scheduler& operator=(const io_scheduler&) = delete;

    // Associate a file handle with this scheduler's IOCP
    bool associate_handle(HANDLE file_handle) {
        HANDLE result = CreateIoCompletionPort(
            file_handle,
            iocp_handle,
            reinterpret_cast<ULONG_PTR>(file_handle),
            0
        );
        return result != nullptr;
    }

    // Register a pending operation and return OVERLAPPED to use
    OVERLAPPED* register_operation(std::coroutine_handle<> cont) {
        auto* overlapped = new OVERLAPPED{};
        {
            std::lock_guard lock(pending_ops_mutex);
            pending_ops[overlapped] = {cont, 0, false};
        }
        return overlapped;
    }

    // Set result for a completed operation
    void set_operation_result(OVERLAPPED* overlapped, std::int64_t result, bool error) {
        std::lock_guard lock(pending_ops_mutex);
        if (auto it = pending_ops.find(overlapped); it != pending_ops.end()) {
            it->second.result = result;
            it->second.is_error = error;
        }
    }

    // Get and clear result for a completed operation
    std::pair<std::int64_t, bool> get_operation_result(OVERLAPPED* overlapped) {
        std::lock_guard lock(pending_ops_mutex);
        if (auto it = pending_ops.find(overlapped); it != pending_ops.end()) {
            auto result = std::make_pair(it->second.result, it->second.is_error);
            pending_ops.erase(it);
            return result;
        }
        return {0, false};
    }

private:
    void dispatch_loop() {
        while (running) {
            DWORD bytes_transferred = 0;
            ULONG_PTR completion_key = 0;
            OVERLAPPED* overlapped = nullptr;

            BOOL success = GetQueuedCompletionStatus(
                iocp_handle,
                &bytes_transferred,
                &completion_key,
                &overlapped,
                500  // 500ms timeout
            );

            if (!overlapped) continue;  // Timeout or shutdown signal

            {
                std::lock_guard lock(pending_ops_mutex);
                if (auto it = pending_ops.find(overlapped); it != pending_ops.end()) {
                    it->second.result = success ? static_cast<std::int64_t>(bytes_transferred) : 0;
                    it->second.is_error = !success;
                    
                    // Resume the waiting coroutine
                    it->second.continuation.resume();
                }
            }
        }
    }
};

// Global scheduler instance
export io_scheduler& get_scheduler() {
    static io_scheduler scheduler;
    return scheduler;
}
