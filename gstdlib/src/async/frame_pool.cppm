export module gs:frame_pool;

import std;
import :types;

// Cache mode for coroutine frame reuse. Used for A/B benchmarking:
//   disabled    - pure ::operator new/delete (baseline)
//   tls_only    - Strategy 1: thread_local LIFO, zero contention
//   shared_only - Strategy 2: global mutex pool, shares across threads
//                 (matters here: main thread allocates, workers free)
//   hybrid      - TLS fast path with shared overflow (tcmalloc-style)
export enum class frame_cache_mode : std::uint8_t {
	disabled = 0,
	tls_only = 1,
	shared_only = 2,
	hybrid = 3
};

export struct frame_cache_stats {
	std::uint64_t alloc_total = 0;
	std::uint64_t alloc_hit_tls = 0;
	std::uint64_t alloc_hit_shared = 0;
	std::uint64_t alloc_miss = 0;
	std::uint64_t free_total = 0;
	std::uint64_t free_to_tls = 0;
	std::uint64_t free_to_shared = 0;
	std::uint64_t free_dropped = 0;
	std::unordered_map<uint64, uint64> sizes_count;

	double hit_rate() const noexcept {
		if (alloc_total == 0) return 0.0;
		return double(alloc_hit_tls + alloc_hit_shared) / double(alloc_total);
	}
};

// Unified frame cache for all coroutine promise types.
// Segregated by frame size (operator new/delete receive the size),
// so one pool safely serves task<T,B> and sequence<T> frames.
export struct frame_cache {
	static void* alloc(std::size_t n);
	static void release(void* p, std::size_t n) noexcept;
	static void release_unsized(void* p) noexcept;
	static void set_mode(frame_cache_mode m) noexcept;
	static frame_cache_mode get_mode() noexcept;
	static frame_cache_stats snapshot();
	static void reset_stats() noexcept;
	// Drop cached frames (calling thread's TLS + shared pool).
	// Workers keep their own TLS until thread exit; use between A/B runs.
	static void clear() noexcept;
	static void set_capacities(std::size_t tls_cap, std::size_t shared_cap) noexcept;
	// Size-class histogram for diagnostics. Off by default: when enabled,
	// alloc takes the shared mutex on every call, so only enable it for
	// benchmark runs, never for production timing.
	static void set_size_tracking(bool enabled) noexcept;
	static bool size_tracking() noexcept;
};

namespace frame_cache_detail {

using entry = std::pair<void*, std::size_t>;

struct tls_cache {
	std::vector<entry> v;

	~tls_cache() {
		// Thread is going away: return memory to the OS. Do NOT touch the
		// shared pool here (static destruction order vs. worker teardown).
		for (auto& e : v)
			::operator delete(e.first);
	}
};

inline tls_cache& local() {
	thread_local tls_cache c;
	return c;
}

struct pool {
	std::atomic<frame_cache_mode> mode{ frame_cache_mode::hybrid };
	std::atomic<std::size_t> tls_cap{ 128 };
	std::atomic<std::size_t> shared_cap{ 4096 };
	std::atomic_bool track_sizes{ false };

	std::atomic<std::uint64_t> alloc_total{ 0 };
	std::atomic<std::uint64_t> alloc_hit_tls{ 0 };
	std::atomic<std::uint64_t> alloc_hit_shared{ 0 };
	std::atomic<std::uint64_t> alloc_miss{ 0 };
	std::atomic<std::uint64_t> free_total{ 0 };
	std::atomic<std::uint64_t> free_to_tls{ 0 };
	std::atomic<std::uint64_t> free_to_shared{ 0 };
	std::atomic<std::uint64_t> free_dropped{ 0 };

	std::mutex m;
	std::vector<entry> shared; // guarded by m
	std::unordered_map<uint64, uint64> sizes_count;

	static pool& instance() {
		static pool p;
		return p;
	}

	// Find a cached frame of exactly size n, swap-remove it. Returns nullptr if none.
	static void* take_from(std::vector<entry>& v, std::size_t n) noexcept {
		if (!v.empty()) {
			for (std::size_t i = v.size(); i-- > 0;) {
				if (v[i].second == n) {
					void* p = v[i].first;
					v[i] = v.back();
					v.pop_back();
					return p;
				}
			}
		}
		return nullptr;
	}
};

} // namespace frame_cache_detail

// One allocator per frame_cache_mode. Each takes the pool for stats/caps
// and the requested frame size, and returns a usable frame.
namespace frame_cache_alloc {

inline void* alloc_disabled(frame_cache_detail::pool& p, std::size_t n) {
	p.alloc_miss.fetch_add(1, std::memory_order_relaxed);
	return ::operator new(n);
}

inline void* alloc_tls_only(frame_cache_detail::pool& p, std::size_t n) {
	if (void* hit = frame_cache_detail::pool::take_from(frame_cache_detail::local().v, n)) {
		p.alloc_hit_tls.fetch_add(1, std::memory_order_relaxed);
		return hit;
	}
	p.alloc_miss.fetch_add(1, std::memory_order_relaxed);
	return ::operator new(n);
}

inline void* alloc_shared_only(frame_cache_detail::pool& p, std::size_t n) {
	void* hit = nullptr;
	{
		std::lock_guard lock(p.m);
		hit = frame_cache_detail::pool::take_from(p.shared, n);
	}
	if (hit) {
		p.alloc_hit_shared.fetch_add(1, std::memory_order_relaxed);
		return hit;
	}
	p.alloc_miss.fetch_add(1, std::memory_order_relaxed);
	return ::operator new(n);
}

inline void* alloc_hybrid(frame_cache_detail::pool& p, std::size_t n) {
	if (void* hit = frame_cache_detail::pool::take_from(frame_cache_detail::local().v, n)) {
		p.alloc_hit_tls.fetch_add(1, std::memory_order_relaxed);
		return hit;
	}
	void* shared_hit = nullptr;
	{
		std::lock_guard lock(p.m);
		shared_hit = frame_cache_detail::pool::take_from(p.shared, n);
	}
	if (shared_hit) {
		p.alloc_hit_shared.fetch_add(1, std::memory_order_relaxed);
		return shared_hit;
	}
	p.alloc_miss.fetch_add(1, std::memory_order_relaxed);
	return ::operator new(n);
}

} // namespace frame_cache_alloc

// One releaser per frame_cache_mode. Each takes the pool plus the frame
// pointer/size, and either caches or deletes the frame.
namespace frame_cache_free {

inline void release_disabled(frame_cache_detail::pool& p, void* ptr, std::size_t) noexcept {
	p.free_dropped.fetch_add(1, std::memory_order_relaxed);
	::operator delete(ptr);
}

inline void release_tls_only(frame_cache_detail::pool& p, void* ptr, std::size_t n) noexcept {
	auto& tls = frame_cache_detail::local().v;
	if (tls.size() < p.tls_cap.load(std::memory_order_relaxed)) {
		try {
			tls.emplace_back(ptr, n);
			p.free_to_tls.fetch_add(1, std::memory_order_relaxed);
			return;
		}
		catch (...) {
			// vector allocation failed: fall through to delete
		}
	}
	p.free_dropped.fetch_add(1, std::memory_order_relaxed);
	::operator delete(ptr);
}

inline void release_shared_only(frame_cache_detail::pool& p, void* ptr, std::size_t n) noexcept {
	bool cached = false;
	{
		std::lock_guard lock(p.m);
		if (p.shared.size() < p.shared_cap.load(std::memory_order_relaxed)) {
			try {
				p.shared.emplace_back(ptr, n);
				cached = true;
			}
			catch (...) {
				cached = false;
			}
		}
	}
	if (cached) {
		p.free_to_shared.fetch_add(1, std::memory_order_relaxed);
		return;
	}
	p.free_dropped.fetch_add(1, std::memory_order_relaxed);
	::operator delete(ptr);
}

inline void release_hybrid(frame_cache_detail::pool& p, void* ptr, std::size_t n) noexcept {
	auto& tls = frame_cache_detail::local().v;
	if (tls.size() < p.tls_cap.load(std::memory_order_relaxed)) {
		try {
			tls.emplace_back(ptr, n);
			p.free_to_tls.fetch_add(1, std::memory_order_relaxed);
			return;
		}
		catch (...) {
			// vector allocation failed: overflow into shared
		}
	}
	// TLS full: overflow into shared
	bool cached = false;
	{
		std::lock_guard lock(p.m);
		if (p.shared.size() < p.shared_cap.load(std::memory_order_relaxed)) {
			try {
				p.shared.emplace_back(ptr, n);
				cached = true;
			}
			catch (...) {
				cached = false;
			}
		}
	}
	if (cached) {
		p.free_to_shared.fetch_add(1, std::memory_order_relaxed);
		return;
	}
	p.free_dropped.fetch_add(1, std::memory_order_relaxed);
	::operator delete(ptr);
}

} // namespace frame_cache_free

void* frame_cache::alloc(std::size_t n) {
	auto& p = frame_cache_detail::pool::instance();
	p.alloc_total.fetch_add(1, std::memory_order_relaxed);
	auto const mode = p.mode.load(std::memory_order_relaxed);

	// Diagnostic only: skip the shared-mutex round trip unless size
	// tracking was explicitly enabled. An unconditional lock here would
	// serialize all allocations - including the TLS fast path and the
	// disabled-mode baseline - on one mutex.
	if (p.track_sizes.load(std::memory_order_relaxed)) {
		std::lock_guard lock(p.m);
		p.sizes_count[n] += 1;
	}

	switch (mode) {
	case frame_cache_mode::tls_only:
		return frame_cache_alloc::alloc_tls_only(p, n);
	case frame_cache_mode::shared_only:
		return frame_cache_alloc::alloc_shared_only(p, n);
	case frame_cache_mode::hybrid:
		return frame_cache_alloc::alloc_hybrid(p, n);
	case frame_cache_mode::disabled:
	default:
		return frame_cache_alloc::alloc_disabled(p, n);
	}
}

void frame_cache::release(void* ptr, std::size_t n) noexcept {
	auto& p = frame_cache_detail::pool::instance();
	p.free_total.fetch_add(1, std::memory_order_relaxed);
	auto const mode = p.mode.load(std::memory_order_relaxed);

	switch (mode) {
	case frame_cache_mode::tls_only:
		return frame_cache_free::release_tls_only(p, ptr, n);
	case frame_cache_mode::shared_only:
		return frame_cache_free::release_shared_only(p, ptr, n);
	case frame_cache_mode::hybrid:
		return frame_cache_free::release_hybrid(p, ptr, n);
	case frame_cache_mode::disabled:
	default:
		return frame_cache_free::release_disabled(p, ptr, n);
	}
}

void frame_cache::release_unsized(void* ptr) noexcept {
	// Size unknown: cannot safely reuse across size classes.
	auto& p = frame_cache_detail::pool::instance();
	p.free_total.fetch_add(1, std::memory_order_relaxed);
	p.free_dropped.fetch_add(1, std::memory_order_relaxed);
	::operator delete(ptr);
}

void frame_cache::set_mode(frame_cache_mode m) noexcept {
	frame_cache_detail::pool::instance().mode.store(m, std::memory_order_relaxed);
}

frame_cache_mode frame_cache::get_mode() noexcept {
	return frame_cache_detail::pool::instance().mode.load(std::memory_order_relaxed);
}

frame_cache_stats frame_cache::snapshot() {
	auto& p = frame_cache_detail::pool::instance();
	frame_cache_stats s{};
	s.alloc_total = p.alloc_total.load(std::memory_order_relaxed);
	s.alloc_hit_tls = p.alloc_hit_tls.load(std::memory_order_relaxed);
	s.alloc_hit_shared = p.alloc_hit_shared.load(std::memory_order_relaxed);
	s.alloc_miss = p.alloc_miss.load(std::memory_order_relaxed);
	s.free_total = p.free_total.load(std::memory_order_relaxed);
	s.free_to_tls = p.free_to_tls.load(std::memory_order_relaxed);
	s.free_to_shared = p.free_to_shared.load(std::memory_order_relaxed);
	s.free_dropped = p.free_dropped.load(std::memory_order_relaxed);
	{
		std::lock_guard lock(p.m);
		s.sizes_count = p.sizes_count;
	}
	return s;
}

void frame_cache::reset_stats() noexcept {
	auto& p = frame_cache_detail::pool::instance();
	p.alloc_total.store(0, std::memory_order_relaxed);
	p.alloc_hit_tls.store(0, std::memory_order_relaxed);
	p.alloc_hit_shared.store(0, std::memory_order_relaxed);
	p.alloc_miss.store(0, std::memory_order_relaxed);
	p.free_total.store(0, std::memory_order_relaxed);
	p.free_to_tls.store(0, std::memory_order_relaxed);
	p.free_to_shared.store(0, std::memory_order_relaxed);
	p.free_dropped.store(0, std::memory_order_relaxed);
}

void frame_cache::clear() noexcept {
	// Calling thread's TLS.
	try {
		auto& tls = frame_cache_detail::local().v;
		for (auto& e : tls)
			::operator delete(e.first);
		tls.clear();
	}
	catch (...) {}
	// Shared pool.
	try {
		auto& p = frame_cache_detail::pool::instance();
		std::vector<frame_cache_detail::entry> tmp;
		{
			std::lock_guard lock(p.m);
			tmp.swap(p.shared);
		}
		for (auto& e : tmp)
			::operator delete(e.first);
	}
	catch (...) {}
}

void frame_cache::set_capacities(std::size_t tls_cap, std::size_t shared_cap) noexcept {
	frame_cache_detail::pool::instance().tls_cap.store(tls_cap, std::memory_order_relaxed);
	frame_cache_detail::pool::instance().shared_cap.store(shared_cap, std::memory_order_relaxed);
}

void frame_cache::set_size_tracking(bool enabled) noexcept {
	frame_cache_detail::pool::instance().track_sizes.store(enabled, std::memory_order_relaxed);
}

bool frame_cache::size_tracking() noexcept {
	return frame_cache_detail::pool::instance().track_sizes.load(std::memory_order_relaxed);
}
