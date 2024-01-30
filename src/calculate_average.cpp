#include <algorithm>
#include <execution>
#include <fstream>
#include <future>
#include <limits>
#include <string>
#include <vector>
#include <iostream>
#include <cstdint>
#include <cstdio>
#include <ranges>
#include <string_view>
#include <print>
#include <unordered_map>
#include <numeric>

#include "file_view.h"
#include "hash_map.h"

/*
 *  CPU: Intel Core i7-10750H 2.6 GHz
 *  RAM: 32 GB DDR4
 *  
 */

/*  Changelist
 *
 *  Initial impl:                       1b - too long
 *  Memory-mapped I/O + views/ranges:   1b - 6.381s
 *  Multithreading:                     1b - 1m2.477s
 *  Forward-only integer parsing:       1b - 34.219s
 *  Improved integer parsing:           1b - 23.470s
 *  Chunk by bytes instead of lines:    1b - 13.249s
 *  std::transform_reduce (par)         1b - 14.986s
 *  std::transform_reduce (par_unseq)   1b - 15.465s
 *  Process chunks on all 12 cores      1b - 11.993s
 *  Custom MultLP hash map              1b - 7.019s
 */

/*  Things to try
 *  SIMD
 *  Large tables
 *  Custom hash
 */

/*  Things that I tried
 *  std::transform_reduce w/ par exec policy - slower than std::async, less CPU utilization (only 6 cores, not all 12)
 */

inline int_fast16_t ParseTemperature(std::string_view sv) {
    int_fast16_t sign = 1;
    if (sv[0] == '-') {
        sv = sv.substr(1);
        sign *= -1;
    }
    // X.X or -X.X
    if (sv[1] == '.') {
        return sign * (10 * sv[0] + sv[2] - '0' * 11);
    }
    // XX.X or -XX.X
    return sign * (100 * sv[0] + 10 * sv[1] + sv[3] - '0' * 111);
}

StatsMap ProcessChunk(std::string_view data, size_t chunk_start,
                      size_t chunk_end) {
    HashMap stats;

    // Recalculate the chunk borders
    // Position the chunk start after the first newline within the current
    // chunk. Position the chunk end at the first newline within the next chunk.
    while (data[chunk_start++] != '\n')
        ;

    for (chunk_end--; chunk_end < data.size() && data[chunk_end] != '\n';
         chunk_end++)
        ;

    std::string_view chunk = data.substr(chunk_start, chunk_end - chunk_start);

    auto lines =
        chunk | std::views::split('\n') |
        std::views::transform([](auto&& str) { return std::string_view{str}; });

    for (std::string_view&& line : lines) {
        auto delim = line.find(';');
        std::string_view id = line.substr(0, delim);
        int_fast16_t measurement =
            ParseTemperature(line.substr(delim + 1, std::string::npos));

        Stats& s = stats[id];
        s.id = id;
        s.max = std::max(s.max, measurement);
        s.min = std::min(s.min, measurement);
        s.n++;
        s.total += measurement;
    }

    return stats.ToStatsMap();
}

void PrintStatsMap(const StatsMap& stats_map) {
    std::vector<std::string_view> keys;
    keys.reserve(stats_map.size());
    for (auto& [k, _] : stats_map) {
        keys.push_back(k);
    }

    std::sort(std::execution::par, keys.begin(), keys.end());
    std::print("{{");
    size_t i = 0;
    for (auto& k : keys) {
        auto& v = stats_map.at(k);
        std::print("{}={:.1f}/{:.1f}/{:.1f}", k, v.min / 10.,
                   v.total / (v.n * 10.), v.max / 10.);
        if (i != keys.size() - 1) [[likely]] {
            std::print(", ");
        };
        i++;
    }
    std::println("}}");
}

int main() {
    const uint_fast32_t num_threads = std::thread::hardware_concurrency();
    // const uint_fast32_t num_threads = 1;

    FileView fv{"measurements.txt"};
    auto mapped_view = fv.file_view;
    auto file_size = mapped_view.length();

    const auto chunk_size = file_size / num_threads;

    // Transform
    std::vector<StatsMap> partial_maps;
    partial_maps.reserve(num_threads);
    {
        std::vector<std::future<StatsMap>> transform_futures;
        transform_futures.reserve(num_threads);
        for (uint32_t i = 0; i < num_threads; i++) {
            uint64_t start = i * chunk_size;
            uint64_t end = std::min((i + 1) * chunk_size, file_size);
            transform_futures.push_back(std::async(
                std::launch::async,
                ProcessChunk,
                mapped_view, start, end
            ));
        }
        for (auto&& f : transform_futures) {
            partial_maps.push_back(f.get());
        }
    }

    // Reduce
    // Done over 6 threads
    StatsMap final_map = std::reduce(
        std::execution::par,
        partial_maps.begin(),
        partial_maps.end(),
        StatsMap{},
        [](auto&& a, auto&& b){
            for (auto [key, stats] : b) {
                auto& [id, min, max, total, n] = a[key];
                id = key;
                min = std::min(min, stats.min);
                max = std::max(max, stats.max);
                total += stats.total;
                n += stats.n;
            }
            return a;
        }
    );

    PrintStatsMap(final_map);
}
