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
#include <chrono>
#include <unordered_map>

#define NOMINMAX  // lol ok
#include <Windows.h>
#include <memoryapi.h>

/*  Changelist
 *
 *  Initial impl:                       1mm - 1.172s    |   1b - too long
 *  Memory-mapped I/O + views/ranges:   1mm - 0.841s    |   1b - 6.381s
 *  Multithreading:                     1mm -           |   1b - 1m2.477s
 *  Forward-only integer parsing:       1mm -           |   1b - 34.219s
 *  Improved integer parsing:           1mm -           |   1b - 23.470s
 */

/*  Things to try
 *  SIMD
 *  Large tables
 *  Custom hash
 */

/*  Things that I tried
 *  std::transform_reduce w/ par exec policy - reduce op needs to be commutative
 * and associative
 */

const int_fast32_t num_lines = 1'000'000'000;

// TODO: check these types
struct Stats {
    int_fast16_t min = std::numeric_limits<int_fast16_t>::max();
    int_fast16_t max = std::numeric_limits<int_fast16_t>::min();
    int_fast64_t total = 0;
    int_fast64_t n = 0;
};

using StatsMap = std::unordered_map<std::string, Stats>;

int_fast16_t ParseTemperature(std::string_view sv) {
    int_fast16_t sign = 1;
    if (sv[0] == '-') {
        sv = sv.substr(1);
        sign *= -1;
    }
    // 1.4 or -2.1
    if (sv[1] == '.') {
        return sign * (10 * sv[0] + sv[2] - '0' * 11);
    }
    // 27.8 or -39.2
    return sign * (100 * sv[0] + 10 * sv[1] + sv[3] - '0' * 111);
}

StatsMap ProcessChunk(auto&& chunk) {
    StatsMap stats;
    for (std::string_view&& line : chunk) {
        auto delim = line.find(';');
        std::string id{line.substr(0, delim)};
        std::string line_s{line};
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
    std::print("{{");
    for (auto& [k, v] : stats_map) {
        std::print("{}={:.1f}/{:.1f}/{:.1f}, ", k, v.min / 10.,
                   v.total / (v.n * 10.), v.max / 10.);
    }
    std::println("}}");
}

void once(const StatsMap& m) {
    static bool done = false;
    if (!done) {
        done = true;
        PrintStatsMap(m);
    }
}

int main() {
    const uint_fast32_t num_threads = std::thread::hardware_concurrency();
    std::println("Running on {} threads", num_threads);

    const auto chunk_size = num_lines / num_threads;

    HANDLE file = CreateFile("measurements.txt", GENERIC_READ, FILE_SHARE_READ,
                             NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    HANDLE mapping =
        CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, "measurements");

    LARGE_INTEGER file_size;
    GetFileSizeEx(file, &file_size);

    const char* mapped = reinterpret_cast<const char*>(
        MapViewOfFile(mapping,
                      FILE_MAP_READ,  // TODO: experiment with large pages
                      0, 0, file_size.QuadPart));

    std::string_view sv{mapped};

    // split da line yaya
    auto chunks =
        sv | std::views::split('\n') | std::views::transform([](auto&& str) {
            return std::string_view(str);
        }) |
        std::views::take(num_lines - 1) | std::views::chunk(chunk_size);

    std::vector<std::future<StatsMap>> thread_results;
    thread_results.reserve(num_threads);
    std::ranges::for_each(chunks, [&thread_results](auto&& chunk) {
        std::future<StatsMap> f = std::async(
            std::launch::async,
            [](auto&& chunk) { return ProcessChunk(chunk); }, chunk);
        thread_results.push_back(std::move(f));
    });

    StatsMap final_map;
    // TODO: sort
    for (auto&& result : thread_results) {
        StatsMap stats_map = result.get();

        for (auto& [id, stats] : stats_map) {
            Stats& final_stats = final_map[id];
            final_stats.total += stats.total;
            final_stats.n += stats.n;
            final_stats.min = std::min(final_stats.min, stats.min);
            final_stats.max = std::max(final_stats.max, stats.max);
        }
    }

    PrintStatsMap(final_map);
}
