#include <algorithm>
#include <fstream>
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

#define NOMINMAX // lol ok
#include <Windows.h>
#include <memoryapi.h>

/*  Changelist
 *  
 *  Initial impl:                       1mm - 1.172s    |   1b - too long
 *  Memory-mapped I/O + views/ranges:   1mm - 0.841s    |   1b - 6.381s
 *  
 */

const int_fast32_t num_lines = 1000000;

// TODO: check these types
struct Stats {
    int_fast32_t min;
    int_fast64_t total;
    int_fast32_t max;
    int_fast64_t n;
};

int main() {

    HANDLE file = CreateFile(
        "measurements_mm.txt",
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    HANDLE mapping = CreateFileMapping(
        file,
        NULL,
        PAGE_READONLY,
        0,
        0,
        "measurements"
    );

    LARGE_INTEGER file_size;
    GetFileSizeEx(
            file,
            &file_size
    );

    const char* mapped = reinterpret_cast<const char*>(MapViewOfFile(
        mapping,
        FILE_MAP_READ, // TODO: experiment with large pages
        0,
        0,
        file_size.QuadPart
    ));

    std::string_view sv{mapped};

    // split da line yaya
    auto lines = sv
        | std::views::split('\n')
        | std::views::transform([](auto&& str) { return std::string_view(str); })
        | std::views::take(num_lines - 1); // can we avoid this or make something more robust?

    std::map<std::string, Stats> stats;

    for (auto [i, line]: std::views::enumerate(lines)) {
        // processLine(line);
        std::string line_s{line};
        auto delim = line_s.find(';');
        std::string id = line_s.substr(0, delim);
        int_fast32_t measurement = std::stod(line_s.substr(delim + 1, std::string::npos)) * 10;
        try {
        } catch (std::exception e) {
            std::println("wtf is this shit {}: {}", id, line_s.substr(delim + 1, std::string::npos));
        }

        if (!stats.contains(id)) {
            stats[id] = Stats{.min=std::numeric_limits<int_fast32_t>::max(), .total=0, .max=std::numeric_limits<int_fast32_t>::min(), .n=0};
        }

        stats[id].max = std::max(stats[id].max, measurement);
        stats[id].min = std::min(stats[id].min, measurement);
        stats[id].n++;
    }

    std::print("{{");
    for (auto& [k, v] : stats) {
        std::print("{}={:.1f}/{:.1f}/{:.1f}, ", k, v.min / 10., v.total / static_cast<double>(v.n), v.max / 10.);
    }
    std::println("}}");
}
