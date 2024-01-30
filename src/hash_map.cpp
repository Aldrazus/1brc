#include "hash_map.h"
#include <cstring>
#include <print>
#include <utility>

HashMap::HashMap() {
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

const uint64_t HashMap::size_ = 16384;
