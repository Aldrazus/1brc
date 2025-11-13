#include <print>
#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <string_view>
#include <algorithm>
#include <utility>
#include <thread>
#include <filesystem>
#include <unordered_map>
#include <ranges>
#include <vector>

struct Statistics {
    int_fast16_t min = std::numeric_limits<int_fast16_t>::max();
    int_fast16_t max = std::numeric_limits<int_fast16_t>::min();
    int_fast32_t n = 0;
    int_fast64_t total = 0;
};

using StatsMap = std::unordered_map<std::string, Statistics>;

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

std::vector<std::pair<size_t, size_t>> splitFileIntoChunks(const std::string& file_path) {
    const auto fileSize = std::filesystem::file_size(file_path);

    const auto numChunks = std::thread::hardware_concurrency();

    std::vector<std::pair<size_t, size_t>> chunks;
    chunks.resize(numChunks);

    std::println("filesize: {}", fileSize);
    const auto baseChunkSize = fileSize / numChunks;
    const auto remainder = fileSize % numChunks;

    for (int i = 0; i < numChunks; i++) {
        const auto chunkSize = baseChunkSize + (i < remainder ? 1 : 0);
        const size_t start = i == 0 ? 0 : chunks[i-1].second;
        const size_t end = start + chunkSize;
        chunks[i] = {start, end};
    }

    return chunks;
}

StatsMap processChunk(std::string_view data, size_t start, size_t end) {
    StatsMap stats;

    for (; start != 0 && data[start] != '\n'; start++) {}

    for (end--; end < data.size() && data[end] != '\n'; end++) {}

    const auto chunk = data.substr(start, end - start);

    auto lines = chunk | std::views::split('\n') | std::views::transform([](auto&& str) { return std::string_view{str}; });

    for (auto&& line : lines) {
        const char delim = ';';

        const auto delimIdx = line.find(delim);
        const auto name = line.substr(0, delimIdx);
        const auto temp = parseTemperature(std::string_view(line).substr(delimIdx + 1));

        auto& [min, max, n, total] = stats[std::string(name)];
        min = std::min(min, temp);
        max = std::max(max, temp);
        n++;
        total += temp;
    }

    return stats;
}

Statistics merge(const Statistics& a, const Statistics& b) {
    return {
        .min = std::min(a.min, b.min),
        .max = std::max(a.max, b.max),
        .n = a.n + b.n,
        .total = a.total + b.total
    };
}

StatsMap merge(const StatsMap& a, const StatsMap& b) {
    auto rv = a;
    
    for (const auto& [k, v] : b) {
        rv[k] = merge(rv[k], v);
    }

    return rv;
}


int main() {
    const auto chunks = splitFileIntoChunks("data/measurements.txt");
    
    for (const auto& [start, end] : chunks) {
        std::println("{} to {}", start, end);
    }

    std::ifstream file("data/measurements.txt");
    if (!file) {
        std::println(std::cerr, "Error opening file!");
        return 1;
    }

    std::string fileContent( (std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));

    file.close();

    StatsMap stats;

    for (const auto& [start, end]: chunks) {
        const auto s = processChunk(fileContent, start, end);
        stats = merge(stats, s);
    }
    
    std::print("{{");

    std::vector<std::string> sorted_strings;

    for (const auto& [k, _] : stats) {
        sorted_strings.push_back(k);
    }

    std::sort(sorted_strings.begin(), sorted_strings.end());

    bool first = true;
    for (const auto& k : sorted_strings) {
        if (!first) {
            std::print(", ");
        }
        const auto& v = stats[k];
        first = false;
        std::print("{}={}/{:.1f}/{}", k, (float)v.min/10.f, (float)v.total/v.n/10.f, (float)v.max/10.f);
    }
    std::println("}}");
}
