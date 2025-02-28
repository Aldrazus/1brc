#include <print>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <sstream>

struct Statistics {
    int_fast16_t min;
    int_fast16_t max;
    int_fast32_t n = 0;
    int_fast64_t total = 0;
};

int main() {
    std::ifstream file("data/measurements.txt");
    if (!file) {
        std::println(std::cerr, "Error opening file!");
        return 1;
    }

    std::unordered_map<std::string, Statistics> stats;
    
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string text;
        int number;
        char delimiter;
        
        if (std::getline(ss, text, ';') && ss >> number) {

        }
    }

    file.close();
}
