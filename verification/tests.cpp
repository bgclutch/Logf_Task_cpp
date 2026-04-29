#include "loglib.hpp"
#include "benchlib.hpp"

#include <iostream>
#include <cmath>
#include <random>
#include <vector>
#include <algorithm>

int main() {
    constexpr size_t size = 10000;
    std::vector<float> data(size);
    std::mt19937 gen(42);

    float input[size];
    float res[size];

    std::generate(data.begin(), data.end(), gen);
    for (size_t i = 0; i < size; ++i) {
        input[i] = data[i];
    }

    auto mylogf_vec = [](float* x, float* res) {
        return math::logf_avx(x, res, size);
    };

    auto stdlog = [](double x) {
        return std::log(x);
    };

    auto mylogf = [](double x) {
        return math::logf(x);
    };

    std::cout << "stdlog res: " << "\n";
    benchlib::funcLatencyTest(stdlog, data);
    benchlib::funcThroughputTest(stdlog, data);
    std::cout << "-------------------------------\n";

    std::cout << "mylogf res: " << "\n";
    benchlib::funcLatencyTest(mylogf, data);
    benchlib::funcThroughputTest(mylogf, data);
    std::cout << "-------------------------------\n";

    std::cout << "mylogf_vec res: " << "\n";
    benchlib::funcLatencyTest(mylogf_vec, data);
    std::cout << "-------------------------------\n";

    return EXIT_SUCCESS;
}