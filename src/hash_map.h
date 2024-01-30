#pragma once

#include <emmintrin.h>
#include <cstdint>
#include <string_view>
#include <unordered_map>

struct Stats {
    std::string_view id;
    int_fast16_t min = std::numeric_limits<int_fast16_t>::max();
    int_fast16_t max = std::numeric_limits<int_fast16_t>::min();
    int_fast64_t total = 0;
    int_fast64_t n = 0;
};

using StatsMap = std::unordered_map<std::string_view, Stats>;

// MultLP hash table
class HashMap {
    public:
        HashMap();
        
        ~HashMap();

        HashMap(const HashMap& other);

        HashMap(HashMap&& other) noexcept;

        HashMap& operator=(HashMap other) noexcept;

        Stats& operator[](std::string_view key);

        StatsMap ToStatsMap() const;

        static uint64_t Hash(std::string_view key);
    private:
        static const uint64_t size_;
        Stats* buckets_;
};

// crc32 sucks, high collisions
// Simple multiply-shift hash function.
// Avoids modular arithmetic on a hash table
// with a size that's a power of two
// h_z(x) = (x * z) / 2^(w-d)
// where x is a w-bit integer, z is an odd w-bit integer,
// the hash table is of size 2^d, and floor division is performed.
// Thanks to integer overflow, x * z will always be in range [0, 2^w)
// Also, floor division by a power of 2 is the same as shifting left
// https://en.wikipedia.org/wiki/Universal_hashing
inline uint64_t HashMap::Hash(std::string_view key) {
    const __m128i* data = reinterpret_cast<const __m128i*>(key.data());
    __m128i chars = _mm_loadu_si128(data);

    __m128i indices =
        _mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
    __m128i mask = _mm_cmplt_epi8(indices, _mm_set1_epi8(std::min(key.length(), 16ull)));

    __m128i masked = _mm_and_si128(chars, mask);
    __m128i sumchars = _mm_add_epi8(masked, _mm_unpackhi_epi64(masked, masked));

    uint64_t x = _mm_cvtsi128_si64(sumchars);

    // Random odd 64-bit unsigned int
    static const uint64_t z = 957877;

    // Take top d bytes of the product to use as the hash
    static const uint32_t w_minus_d = 64 - 14;

    return (x * z) >> w_minus_d;
}

inline StatsMap HashMap::ToStatsMap() const {
    StatsMap m;
    for (uint64_t i = 0; i < size_; i++) {
        Stats& bucket = buckets_[i];
        if (bucket.n == 0) [[unlikely]] {
            continue;
        }
        m[bucket.id] = bucket;
    }
    return m;
}

// Performs linear probing to handle collisions
inline Stats& HashMap::operator[](std::string_view key) {
    auto index = Hash(key);
    Stats& entry = buckets_[index];
    if (entry.n == 0) [[unlikely]] {
        return entry;
    }

    while (entry.n != 0 && entry.id != key) {
        entry = buckets_[++index];
    }

    return entry;
}
