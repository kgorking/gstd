export module gs:sync_console_writer;
import std;
import :Writer;
import :concepts;

// A simple Writer implementation that writes to the console (std::cout) in a thread-safe manner.
class sync_console_writer {
private:
    mutable std::mutex write_mutex;

public:
    std::int64_t write(Span<const char> auto const& data) {
        std::lock_guard<std::mutex> lock(write_mutex);
        std::cout.write(data.data(), data.size());
        return data.size();
    }
};

static_assert(Writer<sync_console_writer>);

// Export a sync_console_writer object.
export sync_console_writer sync_console_writer;
