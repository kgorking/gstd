export module gs:string;
import std;
import :types;
import :utf8;
import :sequence;
import :string_builder;

// Note: This string class stores UTF-8 encoded text as char bytes internally.
// However, all size, indexing, and substring operations work at the CHARACTER level, not byte level.
// This properly handles multi-byte UTF-8 characters (e.g., é = 1 char/2 bytes, 🚀 = 1 char/4 bytes).

struct StringData {
	char const* data = nullptr;
	int64 const size = 0;
	~StringData() {
		delete[] data;
	}
};

// An immutable UTF-8 string class with efficient slicing (substring) and character-level indexing.
export class string {
    std::shared_ptr<StringData> data_;
    int64 start_;
    int64 end_;

public:
    static constexpr int64 npos = -1;

    string() : data_(nullptr), start_(0), end_(0) {}
    string(std::string_view sv) : start_(0), end_(sv.size()) {
        if (!sv.empty()) {
            char *pdata = new char[sv.size()+1];
            std::memcpy(pdata, sv.data(), sv.size());
            pdata[sv.size()] = 0;
            data_ = std::make_shared<StringData>(pdata, sv.size());
        }
    }
    string(const char* s) : start_(0), end_(0) {
        if (s != nullptr) {
            end_ = utf8::byte_length(s);
            char *pdata = new char[end_ + 1];
            std::memcpy(pdata, s, end_);
            pdata[end_] = 0;
			data_ = std::make_shared<StringData>(pdata, end_);
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

    // returns size of characters in bytes
    int64 size() const { 
        return end_ - start_; 
    }

    // returns count of characters, not bytes
    int64 count() const { 
        if (!data_) return 0;
        return utf8::count_chars(c_str(), size_bytes());
    }

    int64 size_bytes() const { 
        return end_ - start_; 
    }

    bool empty() const { return start_ == end_; }

    // Clear the string
    void clear() noexcept {
        data_.reset();
        start_ = end_;
    }

    // Access (checked) - index works with character index, returns the whole UTF-8 character as a int
	int operator[](int64 char_idx) const {
        int64 char_count = count();
        if (char_idx < 0 || char_idx >= char_count) {
            throw std::out_of_range("string index out of range");
        }
        int64 const byte_offset = utf8::char_index_to_byte_offset(c_str(), size_bytes(), char_idx);
        int64 const char_byte_len = utf8::get_char_byte_len(c_str(), byte_offset, size_bytes());
		return utf8::utf8_to_char32(c_str(), byte_offset, char_byte_len);
    }

    // Returns a generator of characters in the string
    sequence<int> chars() const {
        int64 byte_offset = 0;
        while (byte_offset < size_bytes()) {
            int64 char_byte_len = utf8::get_char_byte_len(c_str(), byte_offset, size_bytes());
            co_yield utf8::utf8_to_char32(c_str(), byte_offset, char_byte_len);
            byte_offset += char_byte_len;
        }
    }

    // Substring - works with character positions, not byte positions
    string substr(int64 char_pos, int64 char_len = -1) const {
        int64 byte_pos = utf8::char_index_to_byte_offset(c_str(), size_bytes(), char_pos);
        int64 new_start = start_ + byte_pos;
        
        int64 new_end;
        if (char_len < 0) {
            new_end = end_;
        } else {
            int64 byte_len_needed = utf8::char_index_to_byte_offset(data_->data + new_start, end_ - new_start, char_len);
            new_end = std::min(end_, new_start + byte_len_needed);
        }
        
        return string(data_, new_start, new_end);
    }

    // Remove prefix (character count)
    void remove_prefix(int64 char_count) {
        if (char_count <= 0) return;
        int64 byte_offset = utf8::char_index_to_byte_offset(data_->data + start_, end_ - start_, char_count);
        start_ += byte_offset;
        if (start_ > end_)
            start_ = end_;
    }

    void pop_front() {
        remove_prefix(1);
    }

    // Remove postfix (character count)
    void remove_postfix(int64 char_count) {
        if (char_count <= 0) return;
        int64 char_pos = count() - char_count;
        if (char_pos <= 0) {
            end_ = start_;
            return;
        }
        int64 byte_offset = utf8::char_index_to_byte_offset(data_->data + start_, end_ - start_, char_pos);
        end_ = start_ + byte_offset;
    }

    void pop_back() {
        remove_postfix(1);
    }

    // Get raw data pointer. Needed for the Span concept
    const char* data() const {
        return data_->data + start_;
    }

    // Get C-style string pointer (for compatibility)
    const char* c_str() const {
		return data_->data + start_;
    }

    // Get last character as a string
    string back() const {
        if (empty()) return string();
        
        // Walk backwards from the end to find the start of the last UTF-8 character
        // UTF-8 continuation bytes have pattern 10xxxxxx, so skip those
        int64 last_char_start = end_ - 1;
        while (last_char_start > start_ && (data_->data[last_char_start] & 0xC0) == 0x80) {
            last_char_start--;
        }
        
        return string(data_, last_char_start, end_);
    }

    // Get first character as a string
    string front() const {
        if (empty()) return string();

		int64 const char_len = utf8::char_len(data_->data[start_]);
        return string(data_, start_, std::min(start_ + char_len, end_));
    }

    // Find functions
    int64 find(int c, int64 pos = 0) const {
        if (pos < 0) pos = 0;
        for (int64 i = pos; i < count(); ++i) {
            if ((*this)[i] == c) {
                return i;
            }
        }
        return -1;
    }

    int64 find(string sv, int64 pos = 0) const {
        if (pos < 0) pos = 0;
        if (sv.empty()) return pos <= count() ? pos : -1;
        int64 len = count();
        for (int64 i = pos; i <= len - sv.count(); ++i) {
            bool match = true;
            for (int64 j = 0; j < sv.count(); ++j) {
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
    int64 rfind(int c, int64 pos = -1) const {
        int64 len = count();
        if (pos < 0 || pos >= len) pos = len - 1;
        for (int64 i = pos; i >= 0; --i) {
            if ((*this)[i] == c) {
                return i;
            }
        }
        return -1;
    }

    // Find last occurrence of any character from the given string
    int64 find_last_of(string sv, int64 pos = -1) const {
        int64 len = count();
        if (sv.empty()) return -1;
        if (pos < 0 || pos >= len) pos = len - 1;

        for (int64 i = pos; i >= 0; --i) {
			int const c = (*this)[i];
            for (int64 j = 0; j < sv.count(); ++j) {
                if (sv[j] == c) {
                    return i;
                }
            }
        }
        return -1;
    }

    // Find last occurrence of any character from the given string
    int64 find_last_of(int c, int64 pos = -1) const {
		int64 len = count();
        if (pos < 0 || pos >= len) pos = len - 1;

        for (int64 i = pos; i >= 0; --i) {
            if ((*this)[i] == c) {
                return i;
            }
        }
        return -1;
    }

    // Starts with
    bool starts_with(string sv) const {
        if (sv.size_bytes() > size_bytes()) return false;
        return std::memcmp(c_str(), sv.c_str(), sv.size_bytes()) == 0;
    }

    bool starts_with(int c) const {
        return !empty() && (*this)[0] == c;
    }

    // Ends with
    bool ends_with(string sv) const {
        if (sv.size_bytes() > size_bytes()) return false;
        return std::memcmp(data_->data + end_ - sv.size_bytes(), sv.c_str(), sv.size_bytes()) == 0;
    }

    bool ends_with(int c) const {
        return !empty() && (*this)[count() - 1] == c;
    }

	template <std::integral T = int64>
	T to_int() const {
		T value = 0;
		auto [ptr, ec] = std::from_chars(c_str(), c_str() + size_bytes(), value);
		if (ec != std::errc{})
			throw std::system_error(std::make_error_code(ec));
		return value;
	}

	template <std::floating_point T = float>
	T to_float() const {
		T value = 0;
		auto [ptr, ec] = std::from_chars(c_str(), c_str() + size_bytes(), value);
		if (ec != std::errc{})
			throw std::system_error(std::make_error_code(ec));
		return value;
	}

	// Concatenation
    string operator+(const string& other) const {
        int64 total_size = size_bytes() + other.size_bytes();
        char* new_data = new char[total_size + 1];
        std::memcpy(new_data, c_str(), size_bytes());
        std::memcpy(new_data + size_bytes(), other.c_str(), other.size_bytes());
        new_data[total_size] = 0;
        auto block = std::make_shared<StringData>(new_data, total_size);
        return string(block, 0, total_size);
    }

    string operator+(char c) const {
        int64 total_size = size_bytes() + 1;
        char* new_data = new char[total_size + 1];
        std::memcpy(new_data, c_str(), size_bytes());
        new_data[size_bytes()] = c;
        new_data[total_size] = 0;
        auto block = std::make_shared<StringData>(new_data, total_size);
        return string(block, 0, total_size);
    }

    string operator+(const char* s) const {
        return *this + string(s);
    }

    // Comparison
    bool operator==(string const& other) const {
        if (data_ && other.data_ && data_->data == other.data_->data) {
            bool const same_slice = start_ == other.start_ && end_ == other.end_;
            if (same_slice)
                return true;
        }

        bool const same_size = size_bytes() == other.size_bytes();
        if (!same_size)
            return false;
        return 0 == std::memcmp(c_str(), other.c_str(), size_bytes());
    }

    bool operator==(const int c) const {
        return (*this)[0] == c;
    }

    bool operator==(std::string_view sv) const {
        return std::string_view(data_->data + start_, size_bytes()) == sv;
    }

    bool operator==(const char* s) const {
        auto const s_len = static_cast<int64>(std::strlen(s));
        return size_bytes() == s_len && 0 == std::memcmp(c_str(), s, s_len);
    }

    // Iterator support
    sequence<int> begin() const { return chars(); }
    std::default_sentinel_t end() const noexcept { return {}; }

    // Private constructor for substr
    string(std::shared_ptr<StringData> data, int64 start, int64 end)
        : data_(data), start_(start), end_(end) {}

    string(string_builder&& builder) : data_(nullptr), start_(0), end_(0) {
        if (builder.size > 0 && builder.buffer) {
            // Add null terminator to the builder's buffer
            builder.buffer[builder.size] = 0;
            // Transfer ownership of the buffer directly
            data_ = std::make_shared<StringData>(builder.buffer, builder.size);
            end_ = builder.size;
            // Clear the builder's buffer so its destructor doesn't delete it
            builder.buffer = nullptr;
        } else {
            data_ = nullptr;
            end_ = 0;
        }
    }
};
static_assert(Span<string, const char>, "string should satisfy the Span concept");

// Non-member operators
export std::ostream& operator<<(std::ostream& os, const string& str) {
    os << str.c_str();
    return os;
}

// Formatter specialization for std::format support
export template <> struct std::formatter<::string, char> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(::string const& s, std::format_context& ctx) const {
        auto out = ctx.out();
        for (int64 i = 0; i < s.size_bytes(); ++i) {
            *out++ = static_cast<char>(s.c_str()[i]);
        }
        return out;
    }
};

export template <> struct std::formatter<int, char> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(int const s, std::format_context& ctx) const {
        auto out = ctx.out();
		char buff[4] = { 0 };
		int64 const len = utf8::encode_code_point(s, buff, 4);
        for (int64 i = 0; i < len; ++i) {
            *out++ = buff[i];
        }
        return out;
    }
};

export template<>struct std::formatter<char8_t, char> : std::formatter<char, char> {
	auto format(char8_t c, auto& ctx) const {
		return std::formatter<char, char>::format(static_cast<char>(c), ctx);
	}
};

// Specialization of std::hash for string
export template <> struct std::hash<::string> {
	int64 operator()(const ::string& s) const noexcept {
        int64 h = 0;
        for (int64 i = 0; i < s.size_bytes(); ++i) {
            h = h * 31 + static_cast<unsigned char>(s.c_str()[i]);
        }
        return h;
    }
};
