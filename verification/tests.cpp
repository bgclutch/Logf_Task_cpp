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
    std::vector<float> res(size);
    std::mt19937 gen(42);

    std::generate(data.begin(), data.end(), gen);

    auto mylogf_vec = [&](std::vector<float> x, std::vector<float> out, const size_t dataSize) {
        return math::logf_avx(x.data(), out.data(), dataSize);
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
    benchlib::vfuncTest(mylogf_vec, data);
    std::cout << "-------------------------------\n";

    return EXIT_SUCCESS;
}