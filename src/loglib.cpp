#include "loglib.hpp"

#include <cstdint>
#include <cstring>
#include <cmath>
#include <cfenv>
#include <cerrno>

namespace math {
    namespace detail {
    float log_newton(float x) {
        if (x <= 0)
            return -INFINITY;

        float y = 0.0f;
        // y_0 - начальное условие
        // f(y) = exp(y) - x, обратная функция для y = ln(x)
        // f'(y) = exp(y)
        // y_new = y - (exp(y) - x) / exp(y) = y - 1 + x * exp(-y)
        for (int i = 0; i != 10; ++i) {
            y = y - 1.0f + x * std::exp(-y);
        }
        return y;
    }

    void initLookUpTables() {
        for (int i = 0; i != 256; ++i) {
            float c = 1.0 + static_cast<float>(i) / 256.0;
            R_TABLE[i] = 1.0 / c;
            // T_i = ln(1/R_i) = -ln(R_i) = ln(c)
            T_TABLE[i] = log_newton(c);
        }
    }
    } // namespace detail

    float logf (float x) {
        unsigned int ix;
        std::memcpy(&ix, &x, 4);
        if (std::isnan(x))
            return NAN;

        if (std::isinf(x)) {
            if (x < 0) {
                errno = EDOM;
                std::feraiseexcept(FE_INVALID);
                return NAN;
            }
            return x;
        }

        if (x < 0.0f) {
            errno = EDOM;
            std::feraiseexcept(FE_INVALID);
            return NAN;
        }

        if (x == 0.0f) {
            errno = ERANGE;
            std::feraiseexcept(FE_DIVBYZERO);
            return -INFINITY;
        }

        // Denormal numbers (Bonus part)
        int n = 0;
        if (ix < 0x00800000) {
            x *= 8388608.0f;
            std::memcpy(&ix, &x, 4);
            n -= 23;
        }

        n += static_cast<int>(ix >> 23) - 127;

        unsigned int mantissa = (ix & 0x007FFFFF) | 0x3F800000;
        float x_0;
        std::memcpy(&x_0, &mantissa, 4);

        unsigned int idx = (mantissa & 0x007FFFFF) >> 15;
        float Ri = R_TABLE[idx];
        float Ti = T_TABLE[idx];

        float r = Ri * x_0 - 1.0f;
        float poly = r * (POLY_1 + r * (POLY_2 + r * POLY_3));

        float res = static_cast<float>(n) * LOG_2 + Ti + poly;

        return res;
    }
} // namespace math