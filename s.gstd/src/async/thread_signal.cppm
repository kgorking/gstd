export module gs:thread_signal;

import std;
import :types;
import :bit;

export class thread_signal {
private:
	std::vector<std::unique_ptr<std::atomic_uint8_t>> worker_free{};

public:
	explicit thread_signal(int64 num_signals) {
		worker_free.resize((num_signals + 7) / 8);
		for (auto& w : worker_free)
			w = std::make_unique<std::atomic_uint8_t>(uint8{});
	}

	thread_signal(const thread_signal&) = delete;
	thread_signal& operator=(const thread_signal&) = delete;

	uint64 get() {
		while (true) {
			for (uint64 i = 0; i < worker_free.size(); ++i) {
				std::atomic_uint8_t& mask = *worker_free[i];
				if (!mask) continue;
				uint8 const mod = mask.fetch_xor(mask - 1);
				uint64 const index = std::countr_zero(mod);
				if (index < 8) {
					return (i * 8) + index;
				}
			}
		}
	}

	void clear(uint64 index) {
		if (index >= worker_free.size() * 8)
			throw std::out_of_range("Index out of range in thread_signal::clear");
		*worker_free[index / 8] &= ~(1 << (index % 8));
	}

	void set(uint64 index) {
		if (index >= worker_free.size() * 8)
			throw std::out_of_range("Index out of range in thread_signal::set");
		*worker_free[index / 8] |= 1 << (index % 8);
	}
};
