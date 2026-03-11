export module gs:sync_console_writer;
import std;
import :Writer;

// A simple Writer implementation that writes to the console (std::cout) in a thread-safe manner.
class sync_console_writer {
private:
    mutable std::mutex write_mutex;

public:
    std::expected<std::int64_t, std::error_code> write(std::span<const char> data) {
        std::lock_guard<std::mutex> lock(write_mutex);
        std::cout.write(data.data(), data.size());
        return data.size();
    }
};

static_assert(Writer<sync_console_writer>);

// Export a sync_console_writer object.
export sync_console_writer sync_console_writer;
