#include "loglib.hpp"
#include <iostream>
#include <math.h>

int main() {
    float x;
    std::cin >> x;

    auto result = math::logf(x);
    std::cout << "math::logf result: " << result << "\n"
              << "std::log result:   " << std::log(x) << "\n";

    return EXIT_SUCCESS;
}