export module gs:bit;
import :types;
import std;

export template <unsigned N>
int countr_one(std::bitset<N> const& bs) {
	using Word = uint64;
	constexpr Word word_bits = sizeof(Word) * 8;
	constexpr Word num_words = (N / word_bits) + !std::has_single_bit(N);

	auto const& data = *reinterpret_cast<const std::array<Word, num_words>*>(&bs);
	int sum = 0;

	for (uint64 i = 0; i < num_words; ++i) {
		Word w = data[i];
		sum += std::countr_one(w);
		if (w != ~Word{ 0 }) {
			break;
		}
	}

	return sum;
}

namespace std {
	export constexpr int countr_one(std::bitset<64> const& bs) {
		return std::countr_one(bs.to_ullong());
	}
}

export template <unsigned N>
int countr_zero(std::bitset<N> const& bs) {
	using Word = uint64;
	constexpr Word word_bits = sizeof(Word) * 8;
	constexpr Word num_words = (N / word_bits) + !std::has_single_bit(N);

	auto const& data = *reinterpret_cast<const std::array<Word, num_words>*>(&bs);
	int sum = 0;

	for (uint64 i = 0; i < num_words; ++i) {
		Word w = data[i];
		if constexpr (!std::has_single_bit(N)) {
			if (i == num_words - 1) {
				w |= ~((1 << (word_bits - (num_words * word_bits - N))) - 1);
			}
		}

		sum += std::countr_zero(w);
		if (w != Word{ 0 }) {
			break;
		}
	}

	return sum;
}

#if 0//def __AVX512F__
// Untested
#include <immintrin.h>
template<>
inline int countr_one<512>(const std::bitset<512>& bs) {
	const auto* data = reinterpret_cast<const uint64_t*>(&bs);
	__m512i v = _mm512_load_si512(data);

	// Invert bits: trailing ones become trailing zeros
	__m512i inverted = _mm512_xor_si512(v, _mm512_set1_epi64(-1));

	// Count trailing zeros using AVX-512 VPOPCNT + mask trick
	// Step 1: Isolate lowest set bit
	__m512i neg = _mm512_sub_epi64(_mm512_setzero_si512(), inverted);
	__m512i lowest_bit = _mm512_and_si512(inverted, neg);

	// Step 2: Create mask of bits below lowest zero
	__m512i mask = _mm512_sub_epi64(lowest_bit, _mm512_set1_epi64(1));

	// Step 3: Count bits in mask (popcount)
	__m512i result = _mm512_popcnt_epi64(mask);

	// Step 4: Compare to -1
	__mmask8 cmp = _mm512_cmpeq_epi64_mask(result, _mm512_set1_epi64(-1));

	// Step 5: Compute mask for reduce
	cmp = cmp & -cmp;
	cmp = cmp - 1;

	// Step 6: Horizontal add
	result = _mm512_mask_and_epi64(_mm512_set1_epi64(0), cmp, result, _mm512_set1_epi64(-1));
	return static_cast<int>(_mm512_reduce_add_epi64(result));
	//return static_cast<int>(_mm512_mask_reduce_add_epi64(result, cmp));
}
#endif
