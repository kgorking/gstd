export module gs:utf8;
import std;

export namespace utf8 {
	// Check for a valid lead byte
	inline bool valid_lead_byte(char first_byte) noexcept {
		int const count = std::countl_one(static_cast<std::uint8_t>(first_byte));
		return count != 1 && count <= 4;
	}
	
	// Get the byte length of a UTF-8 character from its first byte
    inline std::ptrdiff_t char_len(char first_byte) noexcept {
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

	// Convert utf-8 byte data to a char32_t
	inline char32_t utf8_to_char32(const char* data, std::ptrdiff_t byte_offset, std::ptrdiff_t char_byte_len) noexcept {
		char32_t codepoint = 0;
		for (int i = 0; i < char_byte_len; i++) {
			codepoint <<= 8;
			codepoint |= static_cast<unsigned char>(data[byte_offset + i]);
		}
		return codepoint;
	}


	// Find the byte offset of the last UTF-8 character start in a null-terminated string. Returns -1 if empty or null.
	inline std::ptrdiff_t find_last_char_start(const char* data, std::ptrdiff_t byte_len) {
		if (!data || data[0] == '\0') return -1;

		// Move backward over continuation bytes (10xxxxxx)
		auto i = byte_len - 1;
		while (i > 0 && (static_cast<unsigned char>(data[i]) & 0xC0) == 0x80) {
			--i;
		}

		return i;
	}

	// Fast lead-byte length calculation. Valid only for well-formed UTF-8 lead bytes.
	inline constexpr std::ptrdiff_t char_len_from_lead(char first_byte) noexcept {
#if !defined(NDEBUG)
		auto const b = static_cast<std::uint8_t>(first_byte);
		// Continuation bytes (10xxxxxx) are invalid here; lead bytes must be 0xxxxxxx or start with >=2 ones.
		if ((b & 0x80) && ~b != 0x7Fu) { /* valid lead or continuation */ }
#endif
		return std::max(1ll, static_cast<std::ptrdiff_t>(std::countl_one(b)));
	}

	// Count character positions in a byte range. Assumes valid UTF-8.
	[[nodiscard]] inline std::ptrdiff_t count_chars(std::span<const char> data) noexcept {
		std::ptrdiff_t chars = 0;
		for (auto const c : data) {
			chars++;
			std::ptrdiff_t const len = char_len_from_lead(c);
			data = data.subspan(len > std::ssize(data) ? 1uz : static_cast<size_t>(len));
		}
		return chars;
	}

	// Convert character index to byte offset. Clamps to buffer end if idx out-of-bounds.
	[[nodiscard]] inline std::ptrdiff_t char_index_to_byte_offset(std::span<const char> data, std::ptrdiff_t char_idx) noexcept {
		if (char_idx < 0) return 0;
		std::ptrdiff_t offset = 0;
		for (std::ptrdiff_t i = 0; i < char_idx && offset < std::ssize(data); ++i) {
			auto const len = char_len_from_lead(data[offset]);
			offset += std::min(len, static_cast<std::ptrdiff_t>(data.size()) - offset);
		}
		return offset;
	}

	// Decode a code point at byte_offset. Updates offset forward. Returns U+FFFD on error.
	[[nodiscard]] inline std::uint32_t decode_code_point(std::span<const char> data, std::ptrdiff_t& offset) noexcept {
		if (offset < 0 || offset >= std::ssize(data)) return 0xFFFD; // replacement char

		auto const b = static_cast<std::uint8_t>(data[offset]);
		std::uint32_t cp = 0;
		std::ptrdiff_t len = 1;

		if ((b & 0x80) == 0x00) cp = b;
		else if ((b & 0xE0) == 0xC0) {
			if (offset + 1 >= std::ssize(data)) return 0xFFFD;
			cp = static_cast<std::uint32_t>((b & 0x1F) << 6 | (static_cast<std::uint8_t>(data[offset + 1]) & 0x3F));
			len = 2;
		}
		else if ((b & 0xF0) == 0xE0) {
			if (offset + 2 >= std::ssize(data)) return 0xFFFD;
			cp = static_cast<std::uint32_t>((b & 0x0F) << 12 | (static_cast<std::uint8_t>(data[offset + 1]) & 0x3F) << 6 | (static_cast<std::uint8_t>(data[offset + 2]) & 0x3F));
			len = 3;
		}
		else if ((b & 0xF8) == 0xF0) {
			if (offset + 3 >= std::ssize(data)) return 0xFFFD;
			cp = static_cast<std::uint32_t>((b & 0x07) << 18 | (static_cast<std::uint8_t>(data[offset + 1]) & 0x3F) << 12 |
				(static_cast<std::uint8_t>(data[offset + 2]) & 0x3F) << 6 | (static_cast<std::uint8_t>(data[offset + 3]) & 0x3F));
			len = 4;
		}
		else {
			return 0xFFFD; // illegal/overlong
		}

		offset += len;
		return cp;
	}

	// Encode a code point into buffer. Returns bytes written (0 on overflow).
	[[nodiscard]] inline std::ptrdiff_t encode_code_point(std::uint32_t cp, char* const buf, std::ptrdiff_t capacity) noexcept {
		if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) return 0; // invalid Unicode

		if (capacity < 1) return 0;

		if (cp <= 0x7F) {
			if (capacity < 1) return 0; buf[0] = static_cast<char>(cp); return 1;
		}
		else if (cp <= 0x7FF) {
			if (capacity < 2) return 0; buf[0] = static_cast<char>(0xC0 | (cp >> 6)); buf[1] = static_cast<char>(0x80 | (cp & 0x3F)); return 2;
		}
		else if (cp <= 0xFFFF) {
			if (capacity < 3) return 0; buf[0] = static_cast<char>(0xE0 | (cp >> 12)); buf[1] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F)); buf[2] = static_cast<char>(0x80 | (cp & 0x3F)); return 3;
		}
		else {
			if (capacity < 4) return 0; buf[0] = static_cast<char>(0xF0 | (cp >> 18)); buf[1] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
			buf[2] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F)); buf[3] = static_cast<char>(0x80 | (cp & 0x3F)); return 4;
		}
	}

	// Fast validation scan. Returns false on malformed sequence or null terminator.
	[[nodiscard]] inline bool validate(std::span<const char> data) noexcept {
		for (; !data.empty(); ) {
			auto const b = static_cast<std::uint8_t>(data[0]);
			if (b == 0x00) return true; // valid end-of-string

			std::ptrdiff_t expected_len = 1;
			if ((b & 0x80)) {
				if ((b & 0xE0) == 0xC0) expected_len = 2;
				else if ((b & 0xF0) == 0xE0) expected_len = 3;
				else if ((b & 0xF8) == 0xF0) expected_len = 4;
				else return false; // illegal lead byte

				if (data.size() < static_cast<size_t>(expected_len)) return false;
				for (std::ptrdiff_t i = 1; i < expected_len; ++i) {
					if ((static_cast<std::uint8_t>(data[i]) & 0xC0) != 0x80) return false; // malformed continuation
				}
			}
			data = data.subspan(expected_len);
		}
		return true;
	}
}