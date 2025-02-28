#include <print>
#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <string_view>
#include <algorithm>
#include <numeric>

struct Statistics {
    int_fast16_t min = std::numeric_limits<int_fast16_t>::max();
    int_fast16_t max = std::numeric_limits<int_fast16_t>::min();
    int_fast32_t n = 0;
    int_fast64_t total = 0;
};

int_fast16_t parseTemperature(std::string_view temp) {
    int_fast16_t sign = 1;
    if (temp[0] == '-') {
        temp = temp.substr(1);
        sign = -1;
    }

    // X.X or -X.X
    if (temp[1] == '.') {
        // Identical to 10 * (temp[0] - '0') + temp[2] - '0'
        return sign * (10 * temp[0] + temp[2] - '0' * 11);
    }

    // XX.X or -XX.X
    return sign * (100 * temp[0] + 10 * temp[1] + temp[3] - '0' * 111);
}

int main() {
    std::ifstream file("data/measurements.txt");
    if (!file) {
        std::println(std::cerr, "Error opening file!");
        return 1;
    }

    std::map<std::string, Statistics> stats;
    
    std::string line;
    while (std::getline(file, line)) {
        const char delim = ';';

        const auto delimIdx = line.find(delim);
        const auto name = line.substr(0, delimIdx);
        const auto temp = parseTemperature(std::string_view(line).substr(delimIdx + 1));

        auto& [min, max, n, total] = stats[name];
        min = std::min(min, temp);
        max = std::max(max, temp);
        n++;
        total += temp;
    }

    file.close();

    std::print("{{");

    bool first = true;
    for (const auto& [k, v] : stats) {
        if (!first) {
            std::print(", ");
        }
        first = false;
        std::print("{}={}/{:.1f}/{}", k, v.min, (float)v.total/v.n, v.max);
    }
    std::println("}}");
}
