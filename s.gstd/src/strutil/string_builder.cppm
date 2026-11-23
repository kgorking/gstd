export module gs:string_builder;
import std;
import :concepts;
import :types;

// Simple helper class for building strings with format
class string_builder {
public:
	using value_type = char;
	char* buffer = nullptr;
	int64 size = 0;
	int64 capacity = 0;

	explicit string_builder(int64 initial_capacity = 0) : size(0), capacity(initial_capacity) {
		if (initial_capacity > 0) {
			buffer = new char[initial_capacity + 1];
			buffer[0] = 0;
		}
	}

	~string_builder() {
		delete[] buffer;
	}

	// Support for std::back_insert_iterator
	void push_back(char c) {
		if (size >= capacity) {
			// Grow capacity exponentially
			int64 new_capacity = (capacity == 0) ? 8 : capacity * 2;
			char* new_buffer = new char[new_capacity + 1];
			if (buffer) {
				std::memcpy(new_buffer, buffer, size);
				new_buffer[size] = 0;
				delete[] buffer;
			}
			buffer = new_buffer;
			capacity = new_capacity;
		}
		buffer[size++] = c;
	}

	void push_span(Span<char const> auto span) {
		if (size + std::ssize(span) >= capacity) {
			// Grow capacity exponentially
			int64 new_capacity = (capacity == 0) ? 8 : capacity * 2;
			while (new_capacity < size + std::ssize(span))
				new_capacity *= 2;

			char* new_buffer = new char[new_capacity + 1];
			if (buffer) {
				std::memcpy(new_buffer, buffer, size);
				new_buffer[size] = 0;
				delete[] buffer;
			}
			buffer = new_buffer;
			capacity = new_capacity;
		}
		std::memcpy(buffer + size, span.data(), span.size());
		size += span.size();
	}

	void pop_back() {
		if (size > 0) {
			size--;
			buffer[size] = 0;
		}
	}

	bool empty() const {
		return size == 0;
	}
};

