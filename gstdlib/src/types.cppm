export module gs:types;
import std;

// Type aliases for fixed-width integer types and pointer-sized types
export using intptr = std::intptr_t;
export using int64 = std::int64_t;
export using int32 = std::int32_t;
export using int16 = std::int16_t;
export using int8 = std::int8_t;

export using uintptr = std::uintptr_t;
export using uint64 = std::uint64_t;
export using uint32 = std::uint32_t;
export using uint16 = std::uint16_t;
export using uint8 = std::uint8_t;

export using usize = std::size_t;

export using byte = std::byte;

// Type aliases for floating-point types
#ifdef __STDCPP_FLOAT16_T__
export using float16 = std::float16_t;
#endif
#ifdef __STDCPP_BFLOAT16_T__
export using bfloat16 = std::bfloat16_t;
#endif
#ifdef __STDCPP_FLOAT32_T__
export using float32 = std::float32_t;
#endif
#ifdef __STDCPP_FLOAT64_T__
export using float64 = std::float64_t;
#endif
#ifdef __STDCPP_FLOAT128_T__
export using float128 = std::float128_t;
#endif
