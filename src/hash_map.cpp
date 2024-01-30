#include "hash_map.h"
#include <crc32intrin.h>
#include <cstring>
#include <print>
#include <utility>
#include <array>

HashMap::HashMap() {
    if (!initialized_masks_) {
        InitMasks();
    }
    buckets_ = new Stats[size_];
}

HashMap::~HashMap() {
    delete[] buckets_;
}

HashMap::HashMap(const HashMap& other) {
    buckets_ = new Stats[size_];
    std::memcpy(buckets_, other.buckets_, size_ * sizeof(Stats));
}

HashMap::HashMap(HashMap&& other) noexcept
    : buckets_(std::exchange(other.buckets_, nullptr)) {}

HashMap& HashMap::operator=(HashMap other) noexcept {
    // TODO: is this valid? memory leak? or is the other destructor called?
    std::swap(buckets_, other.buckets_);
    return *this;
}

Stats& HashMap::operator[](std::string_view key) {
    auto index = Hash(key) % size_;
    Stats& entry = buckets_[index];
    if (entry.n == 0) [[unlikely]] {
        return entry;
    }

    while (buckets_[index].n != 0 && buckets_[index].id != key) {
        index = (index + 1) % size_;
    }

    return buckets_[index];
}

StatsMap HashMap::ToStatsMap() const {
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

// crc32 sucks, high collisions
uint64_t HashMap::Hash(std::string_view key) {
    uint32_t sum = 0;
    for (char c : key) {
        sum += c;
    }

    return (sum * 749449) >> 18;
}

void HashMap::InitMasks() {
    first_word_mask_.fill(~0x0ull);
    second_word_mask_.fill(~0x0ull);

    for (uint64_t i = 1; i < 8; i++) {
        // Only get the first i bytes; zero out the rest
        first_word_mask_[i] = (1ull << (i * 8)) - 1;
        second_word_mask_[i] = 0ull;
    }

    for (uint64_t i = 8; i < 16; i++) {
        // Only get the first i bytes in the second word; zero out the rest
        second_word_mask_[i] = (1ull << (i * 8)) - 1;
    }
}

bool HashMap::initialized_masks_ = false;
std::array<uint64_t, 101> HashMap::first_word_mask_;
std::array<uint64_t, 101> HashMap::second_word_mask_;
const uint64_t HashMap::size_ = 16384 * 8;
