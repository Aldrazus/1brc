#pragma once

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

        static void InitMasks();
    private:
        static bool initialized_masks_;
        static std::array<uint64_t, 101> first_word_mask_;
        static std::array<uint64_t, 101> second_word_mask_;
        static const uint64_t size_;
        Stats* buckets_;
};
