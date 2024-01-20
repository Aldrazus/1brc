#include <algorithm>
#include <execution>
#include <fstream>
#include <future>
#include <limits>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <cstdint>
#include <cstdio>
#include <ranges>
#include <string_view>
#include <print>
#include <unordered_map>
#include <numeric>

#define NOMINMAX  // lol ok
#include <Windows.h>
#include <memoryapi.h>

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
 */

/*  Things to try
 *  SIMD
 *  Large tables
 *  Custom hash
 */

/*  Things that I tried
 *  std::transform_reduce w/ par exec policy - slower than std::async, less CPU utilization (only 6 cores, not all 12)
 */

struct Stats {
    int_fast16_t min = std::numeric_limits<int_fast16_t>::max();
    int_fast16_t max = std::numeric_limits<int_fast16_t>::min();
    int_fast64_t total = 0;
    int_fast64_t n = 0;
};

using StatsMap = std::unordered_map<std::string, Stats>;

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
    StatsMap stats;

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
        std::string id{line.substr(0, delim)};
        int_fast16_t measurement =
            ParseTemperature(line.substr(delim + 1, std::string::npos));

        Stats& s = stats[id];
        s.max = std::max(s.max, measurement);
        s.min = std::min(s.min, measurement);
        s.n++;
        s.total += measurement;
    }

    return stats;
}

void PrintStatsMap(const StatsMap& stats_map) {
    std::vector<std::string> keys;
    keys.reserve(stats_map.size());
    for (auto& [k, _] : stats_map) {
        keys.push_back(k);
    }

    std::sort(std::execution::par, keys.begin(), keys.end());
    std::print("{{");
    for (auto& k : keys) {
        auto& v = stats_map.at(k);
        std::print("{}={:.1f}/{:.1f}/{:.1f}, ", k, v.min / 10.,
                   v.total / (v.n * 10.), v.max / 10.);
    }
    std::println("}}");
}

inline std::string_view trim_end(std::string_view sv) {
    if (sv[sv.length() - 1] == '\n') {
        return sv.substr(0, sv.length() - 1);
    }
    return sv;
}

struct Bounds {
    size_t start;
    size_t end;
};

int main() {
    const uint_fast32_t num_threads = std::thread::hardware_concurrency();

    HANDLE file = CreateFile("measurements.txt", GENERIC_READ, FILE_SHARE_READ,
                             NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    HANDLE mapping =
        CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, "measurements");

    LARGE_INTEGER file_size;
    GetFileSizeEx(file, &file_size);

    const auto chunk_size = file_size.QuadPart / num_threads;

    const char* mapped = reinterpret_cast<const char*>(
        MapViewOfFile(mapping,
                      FILE_MAP_READ,  // TODO: experiment with large pages
                      0, 0, file_size.QuadPart));

    std::string_view sv = trim_end(mapped);

    std::vector<Bounds> bounds;
    bounds.reserve(num_threads);
    for (size_t i = 0; i < num_threads - 1; i++) {
        bounds.emplace_back(i * chunk_size, (i + 1) * chunk_size);
    }
    bounds.emplace_back((num_threads - 1) * chunk_size, sv.size());

    StatsMap final_map = std::transform_reduce(
        std::execution::par_unseq,
        bounds.begin(),
        bounds.end(),
        StatsMap{},
        [](StatsMap&& a, const StatsMap&& b){
            for (auto [id, stats] : b) {
                auto& [min, max, total, n] = a[id];
                min = std::min(min, stats.min);
                max = std::max(max, stats.max);
                total += stats.total;
                n += stats.n;
            }
            return a;
        },
        [sv](Bounds b){
            return ProcessChunk(sv, b.start, b.end);
        }
    );

    PrintStatsMap(final_map);
}
