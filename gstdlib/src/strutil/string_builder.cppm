export module gs:string_builder;
import std;
import :types;

// Simple helper class for building strings with format
class string_builder {
public:
	using value_type = char;
	char* buffer = nullptr;
	int64 size = 0;
	int64 capacity = 0;

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

