export module gs:string;

import std;

// Note: This string class stores UTF-8 encoded text as char bytes internally.
// However, all size, indexing, and substring operations work at the CHARACTER level, not byte level.
// This properly handles multi-byte UTF-8 characters (e.g., é = 1 char/2 bytes, 🚀 = 1 char/4 bytes).

struct StringData {
    char const* data;
    std::ptrdiff_t size;
    ~StringData() { 
        delete[] data; 
    }
};

// helper functions for the string class
namespace {
    // Helper class for building strings with format
    class StringBuilder {
    public:
        using value_type = char;
        char* buffer = nullptr;
        std::ptrdiff_t size = 0;
        std::ptrdiff_t capacity = 0;
        
        inline ~StringBuilder() {
            delete[] buffer;
        }
        
        // Support for std::back_insert_iterator
        inline void push_back(char c) {
            if (size >= capacity) {
                // Grow capacity exponentially
                std::ptrdiff_t new_capacity = (capacity == 0) ? 16 : capacity * 2;
                char* new_buffer = new char[new_capacity];
                if (buffer) {
                    std::memcpy(new_buffer, buffer, size);
                    delete[] buffer;
                }
                buffer = new_buffer;
                capacity = new_capacity;
            }
            buffer[size++] = c;
        }
    };

    // Get the byte length of a UTF-8 character from its first byte
    inline std::ptrdiff_t utf8_char_len(char first_byte) {
        return std::max(1, std::countl_one((std::uint8_t)first_byte));
    }

    // Count the number of UTF-8 characters in a byte range
    inline std::ptrdiff_t count_utf8_chars(const char* data, std::ptrdiff_t byte_len) {
        std::ptrdiff_t char_count = 0;
        std::ptrdiff_t i = 0;
        while (i < byte_len) {
            i += utf8_char_len(data[i]);
            char_count++;
        }
        return char_count;
    }

    // Find the length of a UTF-8 string
    inline std::ptrdiff_t utf8_length(const char* data) {
        std::ptrdiff_t char_count = 0;
        std::ptrdiff_t i = 0;
        while (data[i] != 0) {
            i += utf8_char_len(data[i]);
            char_count++;
        }
        return char_count;
    }

    // Find the byte-length of a UTF-8 string
    inline std::ptrdiff_t utf8_byte_length(const char* data) {
        std::ptrdiff_t i = 0;
        while (data[i] != 0) {
            i += 1;
        }
        return i;
    }

    // Convert character index to byte offset
    inline std::ptrdiff_t char_index_to_byte_offset(const char* data, std::ptrdiff_t byte_len, std::ptrdiff_t char_idx) {
        std::ptrdiff_t byte_offset = 0;
        std::ptrdiff_t char_count = 0;
        while (char_count < char_idx) {
            auto const char_len = utf8_char_len(data[byte_offset]);
            if (byte_offset + char_len > byte_len)
                return byte_offset;
            byte_offset += char_len;
            char_count++;
        }
        return byte_offset;
    }

    // Get byte length of character at given byte offset
    inline std::ptrdiff_t get_char_byte_len(const char* data, std::ptrdiff_t byte_offset, std::ptrdiff_t byte_len) {
        if (byte_offset >= byte_len) return 0;
        return utf8_char_len(data[byte_offset]);
    }
}

// An immutable UTF-8 string class with efficient slicing (substring) and character-level indexing.
export class string {
    std::shared_ptr<StringData> data_;
    std::ptrdiff_t start_;
    std::ptrdiff_t end_;

public:
    static constexpr std::ptrdiff_t npos = -1;

    string() : data_(nullptr), start_(0), end_(0) {}
    // Constructor from std::string_view (assumes UTF-8 or ASCII-compatible encoding)
    string(std::string_view sv) : start_(0), end_(sv.size()) {
        if (!sv.empty()) {
            data_ = std::make_shared<StringData>();
            char *pdata = new char[sv.size()+1];
            // Safely reinterpret as UTF-8 bytes (std::string is typically UTF-8 in modern C++)
            std::memcpy(pdata, reinterpret_cast<const char*>(sv.data()), sv.size());
            pdata[sv.size()] = 0;
            data_->data = pdata;
            data_->size = sv.size();
        }
    }
    string(const char* s) : start_(0), end_(0) {
        if (s != nullptr) {
            end_ = utf8_byte_length(s);
            data_ = std::make_shared<StringData>();
            char *pdata = new char[end_ + 1];
            std::memcpy(pdata, s, end_);
            pdata[end_] = 0;
            data_->data = pdata;
            data_->size = end_;
        }
    }
    string(const string&) = default;
    string(string&&) = default;
    ~string() = default;

    // Assignment
    string& operator=(const string&) = default;
    string& operator=(string&&) = default;

    // Assignment from string views and pointers
    string& operator=(std::string_view sv) {
        *this = string(sv);
        return *this;
    }

    string& operator=(const char* s) {
        *this = string(s);
        return *this;
    }

    // Format a string using std::format
    template<typename... Args>
    static string fmt(std::format_string<Args...> format_str, Args&&... args) {
        StringBuilder builder;
        std::format_to(std::back_inserter(builder), format_str, std::forward<Args>(args)...);
        return string(std::move(builder));
    }

    // returns size of characters in bytes
    std::ptrdiff_t size() const { 
        return end_ - start_; 
    }

    // returns count of characters, not bytes
    std::ptrdiff_t count() const { 
        if (!data_) return 0;
        return count_utf8_chars(data(), size_bytes()); 
    }

    std::ptrdiff_t size_bytes() const { 
        return end_ - start_; 
    }

    bool empty() const { return start_ == end_; }

    // Clear the string
    void clear() noexcept {
        data_.reset();
        start_ = end_;
    }

    // Access (checked) - index works with character index, returns the whole UTF-8 character as a view
    string operator[](std::ptrdiff_t char_idx) const {
        std::ptrdiff_t char_count = count();
        if (char_idx < 0 || char_idx >= char_count) {
            throw std::out_of_range("string index out of range");
        }
        std::ptrdiff_t const byte_offset = char_index_to_byte_offset(data(), size_bytes(), char_idx);
        std::ptrdiff_t const char_byte_len = get_char_byte_len(data(), byte_offset, size_bytes());
        return string(data_, start_ + byte_offset, start_ + byte_offset + char_byte_len);
    }

    // Substring - works with character positions, not byte positions
    string substr(std::ptrdiff_t char_pos, std::ptrdiff_t char_len = -1) const {
        std::ptrdiff_t byte_pos = char_index_to_byte_offset(data_->data + start_, end_ - start_, char_pos);
        std::ptrdiff_t new_start = start_ + byte_pos;
        
        std::ptrdiff_t new_end;
        if (char_len < 0) {
            new_end = end_;
        } else {
            std::ptrdiff_t byte_len_needed = char_index_to_byte_offset(data_->data + new_start, end_ - new_start, char_len);
            new_end = std::min(end_, new_start + byte_len_needed);
        }
        
        return string(data_, new_start, new_end);
    }

    // Remove prefix (character count)
    void remove_prefix(std::ptrdiff_t char_count) {
        if (char_count <= 0) return;
        std::ptrdiff_t byte_offset = char_index_to_byte_offset(data_->data + start_, end_ - start_, char_count);
        start_ += byte_offset;
        if (start_ > end_)
            start_ = end_;
    }

    // Remove postfix (character count)
    void remove_postfix(std::ptrdiff_t char_count) {
        if (char_count <= 0) return;
        std::ptrdiff_t char_pos = count() - char_count;
        if (char_pos <= 0) {
            end_ = start_;
            return;
        }
        std::ptrdiff_t byte_offset = char_index_to_byte_offset(data_->data + start_, end_ - start_, char_pos);
        end_ = start_ + byte_offset;
    }

    // Get raw data pointer
    const char* data() const {
        return data_->data + start_;
    }

    // Get C-style string pointer (for compatibility)
    const char* c_str() const {
        return data();
    }

    // Get last character as a string
    string back() const {
        if (empty()) return string();
        
        // Walk backwards from the end to find the start of the last UTF-8 character
        // UTF-8 continuation bytes have pattern 10xxxxxx, so skip those
        std::ptrdiff_t last_char_start = end_ - 1;
        while (last_char_start > start_ && (data_->data[last_char_start] & 0xC0) == 0x80) {
            last_char_start--;
        }
        
        return string(data_, last_char_start, end_);
    }

    // Find functions
    std::ptrdiff_t find(char c, std::ptrdiff_t pos = 0) const {
        if (pos < 0) pos = 0;
        for (std::ptrdiff_t i = pos; i < count(); ++i) {
            if (data_->data[start_ + i] == c) {
                return i;
            }
        }
        return -1;
    }

    std::ptrdiff_t find(string sv, std::ptrdiff_t pos = 0) const {
        if (pos < 0) pos = 0;
        if (sv.empty()) return pos <= count() ? pos : -1;
        std::ptrdiff_t len = count();
        for (std::ptrdiff_t i = pos; i <= len - sv.count(); ++i) {
            bool match = true;
            for (std::ptrdiff_t j = 0; j < sv.count(); ++j) {
                if ((*this)[i + j] != sv[j]) {
                    match = false;
                    break;
                }
            }
            if (match) return i;
        }
        return -1;
    }

    // Reverse find
    std::ptrdiff_t rfind(char c, std::ptrdiff_t pos = -1) const {
        std::ptrdiff_t len = count();
        if (pos < 0 || pos >= len) pos = len - 1;
        for (std::ptrdiff_t i = pos; i >= 0; --i) {
            if ((*this)[i] == c) {
                return i;
            }
        }
        return -1;
    }

    // Starts with
    bool starts_with(string sv) const {
        if (sv.count() > count()) return false;
        return std::memcmp(data_->data + start_, sv.data(), sv.size_bytes()) == 0;
    }

    bool starts_with(char c) const {
        return !empty() && data_->data[start_] == c;
    }

    // Ends with
    bool ends_with(string sv) const {
        if (sv.count() > count()) return false;
        return std::memcmp(data_->data + end_ - sv.size_bytes(), sv.data(), sv.size_bytes()) == 0;
    }

    bool ends_with(char c) const {
        return !empty() && data_->data[end_ - 1] == c;
    }

    // Concatenation
    string operator+(const string& other) const {
        std::ptrdiff_t total_size = size_bytes() + other.size_bytes();
        char* new_data = new char[total_size + 1];
        std::memcpy(new_data, data(), size_bytes());
        std::memcpy(new_data + size_bytes(), other.data(), other.size_bytes());
        new_data[total_size] = 0;
        auto block = std::make_shared<StringData>();
        block->data = new_data;
        block->size = total_size;
        return string(block, 0, total_size);
    }

    string operator+(const char* s) const {
        return *this + string(s);
    }

    // Comparison
    bool operator==(string const& other) const {
        if (data_->data == other.data_->data) {
            bool const same_slice = start_ == other.start_ && end_ == other.end_;
            if (same_slice)
                return true;
        }

        bool const same_size = size_bytes() == other.size_bytes();
        if (!same_size)
            return false;
        return 0 == std::memcmp(data(), other.data(), size_bytes());
    }

    bool operator==(const char c) const {
        return size() > 0 && data_->data[start_] == c;
    }

    bool operator==(const char* s) const {
        auto const s_len = (std::intptr_t)std::strlen(s);
        return size_bytes() == s_len && 0 == std::memcmp(data(), s, s_len);
    }

    // Iterator support
    const char* begin() const { return data_->data + start_; }
    const char* end() const { return data_->data + end_; }

private:
    // Private constructor for substr
    string(std::shared_ptr<StringData> data, std::ptrdiff_t start, std::ptrdiff_t end)
        : data_(data), start_(start), end_(end) {}

    string(StringBuilder&& builder) : start_(0), data_(nullptr), end_(0) {
        if (builder.size > 0 && builder.buffer) {
            data_ = std::make_shared<StringData>();
            // Add null terminator to the builder's buffer
            //builder.buffer[builder.size] = 0;
            // Transfer ownership of the buffer directly
            data_->data = builder.buffer;
            data_->size = builder.size;
            end_ = builder.size;
            // Clear the builder's buffer so its destructor doesn't delete it
            builder.buffer = nullptr;
        } else {
            if (builder.buffer) delete[] builder.buffer;
            data_ = nullptr;
            end_ = 0;
        }
    }
};

// Non-member operators
std::ostream& operator<<(std::ostream& os, const string& str) {
    os << str.data();
    return os;
}
export inline bool operator==(const char* sz, string s) {
    return s == sz;
}

// Formatter specialization for std::format support
export template <>
struct std::formatter<::string, char> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(::string const& s, std::format_context& ctx) const {
        auto out = ctx.out();
        for (std::ptrdiff_t i = 0; i < s.size_bytes(); ++i) {
            *out++ = static_cast<char>(s.data()[i]);
        }
        return out;
    }
};
