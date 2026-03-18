export module gs:utf8;
import std;

export namespace utf8 {
    // Get the byte length of a UTF-8 character from its first byte
    inline std::ptrdiff_t char_len(char first_byte) {
        return std::max(1, std::countl_one(static_cast<std::uint8_t>(first_byte)));
    }

    // Count the number of UTF-8 characters in a byte range
    inline std::ptrdiff_t count_chars(const char* data, std::ptrdiff_t byte_len) {
        std::ptrdiff_t char_count = 0;
        std::ptrdiff_t i = 0;
        while (i < byte_len) {
            i += char_len(data[i]);
            char_count++;
        }
        return char_count;
    }

    // Find the length of a UTF-8 string
    inline std::ptrdiff_t length(const char* data) {
        std::ptrdiff_t char_count = 0;
        std::ptrdiff_t i = 0;
        while (data[i] != 0) {
            i += char_len(data[i]);
            char_count++;
        }
        return char_count;
    }

    // Find the byte-length of a UTF-8 string
    inline std::ptrdiff_t byte_length(const char* data) {
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
            auto const len = char_len(data[byte_offset]);
            if (byte_offset + len > byte_len)
                return byte_offset;
            byte_offset += len;
            char_count++;
        }
        return byte_offset;
    }

    // Get byte length of character at given byte offset
    inline std::ptrdiff_t get_char_byte_len(const char* data, std::ptrdiff_t byte_offset, std::ptrdiff_t byte_len) {
        if (byte_offset >= byte_len) return 0;
        return char_len(data[byte_offset]);
    }
}
