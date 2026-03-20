#include "loglib.hpp"
#include <iostream>
#include <iomanip>
#include <math.h>

int main() {
    float x;
    std::cin >> x;

    math::detail::initLookUpTables();
    auto result = math::logf(x);
    std::cout << std::fixed << std::setprecision(9);
    std::cout << "for x = " << x << "\n"
              << "math::logf result: " << result << "\n"
              << "std::log result:   " << std::log(x) << "\n";

    return EXIT_SUCCESS;
}