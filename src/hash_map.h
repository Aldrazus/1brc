#pragma once

#include <string>
#include <limits>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <memory>
#include <array>
#include <algorithm>

struct Stats {
    std::string id;
    int_fast16_t min = std::numeric_limits<int_fast16_t>::max();
    int_fast16_t max = std::numeric_limits<int_fast16_t>::min();
    int_fast64_t total = 0;
    int_fast64_t n = 0;
};

using StatsMapTemp = std::unordered_map<std::string_view, Stats>;

template<size_t Size = 16384>
class HashMap {
    public:
        HashMap() = default;
        ~HashMap() = default;
        HashMap(const HashMap&) = default;
        HashMap& operator=(const HashMap&) = default;
        HashMap(HashMap&&) = default;
        HashMap& operator=(HashMap&&) = default;

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
        size_t hash(std::string_view key) {
            std::array<char, 8> eightChars = {}; // Initialize all bytes to 0

            size_t keysToCopy = std::min(key.size(), static_cast<size_t>(8));
            std::memcpy(eightChars.data(), key.data(), keysToCopy);

            uint64_t x = *reinterpret_cast<uint64_t*>(eightChars.data());

            // Random odd 64-bit unsigned int
            static const uint64_t z = 0xC2B2AE3D27D4EB4F;

            // Take top d bytes of the product to use as the hash
            static const uint32_t w_minus_d = 64 - 14;

            return (x * z) >> w_minus_d;
        }

        Stats& operator[](std::string_view key) {
            auto index = hash(key);

            Stats& entry = (*buckets_)[index];
            
            if (entry.n == 0) [[unlikely]] {
                return entry;
            }

            while (entry.n != 0 && entry.id != key) {
                entry = (*buckets_)[++index];
            }
            // XXX: what if end of array is reached??
            return entry;
        }
    private:
        std::unique_ptr<std::array<Stats, Size>> buckets_;
};
