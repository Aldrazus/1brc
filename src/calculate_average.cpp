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

/*  Changelist
 *  
 *  Initial impl:  1mm - 1.172s
 *  
 */

// TODO: check these types
struct Stats {
    int_fast32_t min;
    int_fast64_t total;
    int_fast32_t max;
    int_fast64_t n;
};

int main() {
    std::map<std::string, Stats> stats;
    
    std::ifstream ifs{"measurements_mm.txt"};

    std::ofstream ofs{"test.txt"};

    if (!ifs) {
        std::cerr << "Cannot open measurements.txt\n";
    }

    std::string line;
    while (std::getline(ifs, line)) {
        auto delim = line.find(';');
        std::string id = line.substr(0, delim);
        int_fast32_t measurement = std::stod(line.substr(delim + 1, std::string::npos)) * 10;

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
