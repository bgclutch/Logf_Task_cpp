#pragma once

#include "tables.hpp"
#include <cstdint>
#include <cstring>
#include <cmath>

namespace math {
    const float POLY_1 =  0x1.55561555cccd4p-2;
    const float POLY_2 = -0x1.00006000349d3p-1;
    const float POLY_3 =  0x1.fffffffff6666p-1;
    const float LOG_2  =  0x1.62e42fefa39efp-1;

    float logf (float x) {
        unsigned int ix;
        std::memcpy(&ix, &x, 4);
        if (ix >= 0x7F800000 || ix == 0) {
            if (ix == 0x7F800000) return x;
            if (ix == 0x00000000) return -INFINITY;
            return NAN;
        }

        // Denormal Bonus
        int n = 0;
        if (ix < 0x00800000) {
            x *= 8388608.0f;
            std::memcpy(&ix, &x, 4);
            n -= 23;
        }

        n += (ix >> 23) - 127;
        unsigned int mantissa = (ix & 0x007FFFFF) | 0x3F800000;
        float x_0;
        std::memcpy(&x_0, &mantissa, 4);

        unsigned int idx = (ix & 0x007FFFFF) >> 15;
        float  Ri = R_TABLE[idx];
        float Ti = T_TABLE[idx];

        float r = Ri * x_0 - 1.0;
        float poly = r * (POLY_1 + r * (POLY_2 + r * POLY_3));

        float res = n * LOG_2 + Ti + poly;

        return res;
    }
}