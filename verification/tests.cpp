#include "loglib.hpp"
#include "benchlib.hpp"

#include <iostream>
#include <cmath>
#include <random>
#include <vector>
#include <algorithm>

#include <immintrin.h>
#include <sleef.h>
#include <mkl.h>

int main() {
    constexpr size_t size = 10240;
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

    auto sleef_vec = [&](std::vector<float> x, std::vector<float> out, const size_t dataSize) {
        for (size_t i = 0; i < dataSize; i += 8) {
            __m256 vx = _mm256_loadu_ps(&x[i]);
            __m256 vy = Sleef_logf8_u10(vx);
            _mm256_storeu_ps(&out[i], vy);
        }
    };

    auto mkl_vecha = [&](std::vector<float> x, std::vector<float> out, const size_t dataSize) {
        vmsLn(dataSize, x.data(), out.data(), VML_HA);
    };

    auto mkl_vecla = [&](std::vector<float> x, std::vector<float> out, const size_t dataSize) {
        vmsLn(dataSize, x.data(), out.data(), VML_LA);
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

    std::cout << "SLEEF\n";
    benchlib::vfuncTest(sleef_vec, data);
    std::cout << "\n";

    std::cout << "Intel MKL\n";
    benchlib::vfuncTest(mkl_vecha, data);
    std::cout << "\n";

    std::cout << "Intel MKL\n";
    benchlib::vfuncTest(mkl_vecla, data);
    std::cout << "\n";

    return EXIT_SUCCESS;
}