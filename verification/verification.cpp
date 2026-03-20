#include "loglib.hpp"
#include "verification.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <iomanip>
#include <cerrno>
#include <cfenv>

namespace math {
namespace verification {
const std::string fileName = "analysis.csv";

void check_val(float x, std::ofstream& dump) {
    std::feclearexcept(FE_ALL_EXCEPT);
    errno = 0;

    float res = math::logf(x);

    dump << "logf(" << x << ") = " << res << "\n";
    dump << "  errno: " << errno << "\n";
    dump << "  FE_INVALID: " << (std::fetestexcept(FE_INVALID) ? "YES" : "NO") << "\n";
    dump << "  FE_DIVBYZERO: " << (std::fetestexcept(FE_DIVBYZERO) ? "YES" : "NO") << "\n\n";
}

float calc_ulp_error(double ref, float test) {
    if (ref == static_cast<double>(test))
        return 0.0f;

    if (std::isnan(ref) && std::isnan(test))
        return 0.0f;

    if (std::isinf(ref) && std::isinf(test) && (ref == test))
        return 0.0f;

    uint64_t ref_bits;
    std::memcpy(&ref_bits, &ref, sizeof(uint64_t));

    const uint64_t EXP_DOUBLE_MASK = 0x7FF0000000000000ULL;
    uint64_t exp_bits = ref_bits & EXP_DOUBLE_MASK;

    double ulp_val;
    if (exp_bits < (23ULL << 52)) {
        ulp_val = 1e-38;
    } else {
        uint64_t ulp_bits = exp_bits - (23ULL << 52);
        std::memcpy(&ulp_val, &ulp_bits, sizeof(double));
    }

    float ulp_error = static_cast<float>(std::abs(static_cast<double>(test) - ref) / ulp_val);
    return ulp_error;
}

void run_interval_test(float start, float end, size_t points, std::ofstream& csv, std::ofstream& dump) {
    float max_ulp = 0.0f;

    static bool init = false;
    if(!init) {
        math::detail::initLookUpTables();
        init = true;
    }

    for (size_t i = 0; i != points; ++i) {
        float x = start + (end - start) * (static_cast<float>(i) / points);

        double ref = std::log(static_cast<double>(x));
        float test = math::logf(x);
        float ulp = calc_ulp_error(ref, test);

        max_ulp = std::max(max_ulp, math::verification::calc_ulp_error(ref, test));

        if (i % 50 == 0) {
            csv << x << "," << test << "," << ref << "," << ulp << "\n";
        }
    }

    dump << "Interval [" << std::setw(3) << start << ", " << std::setw(3) << end << "] "
         << "| Max ULP Error: " << std::fixed << std::setprecision(9) << max_ulp << "\n";
}
} // namespace verification
} // namespace math

int main() {
    std::ofstream dump("dump.txt");
    math::verification::check_val(-1.0f, dump);
    math::verification::check_val(0.0f, dump);
    math::verification::check_val(INFINITY, dump);
    math::verification::check_val(NAN, dump);

    std::vector<std::pair<float, float>> intervals;
    for (int k = -127; k <= 128; ++k) {
        float start = std::pow(2.0f, static_cast<float>(k));
        float end = std::pow(2.0f, static_cast<float>(k + 1));
        intervals.push_back({start, end});
    }

    std::ofstream csv(math::verification::fileName);
    csv << "x,logf,std::log,ulp_error\n";

    for (const auto& interval : intervals) {
        math::verification::run_interval_test(interval.first, interval.second, 10000, csv, dump);
    }
    csv.close();
    dump.close();

    return EXIT_SUCCESS;
}