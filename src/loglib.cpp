#include "loglib.hpp"

#include <cstdint>
#include <cstring>
#include <cmath>
#include <cfenv>
#include <cerrno>

#include <iostream>
#include <iomanip>

namespace math {
    namespace detail {
    double log_newton(double x) {
        if (x == 0.)
            return -INFINITY;
        if (x < 0.)
            return NAN;

        double y = 0.0f;
        // y_0 - начальное условие
        // f(y) = exp(y) - x, обратная функция для y = ln(x)
        // f'(y) = exp(y)
        // y_new = y - (exp(y) - x) / exp(y) = y - 1 + x * exp(-y)
        for (int i = 0; i != 10; ++i) {
            y = y - 1. + x * std::exp(-y);
        }
        return y;
    }

    void initLookUpTables() {
        for (int i = 0; i != 256; ++i) {
            double c = 1.0 + static_cast<double>(i) / 256.0;
            float tmpRes = static_cast<float>(1. / c);
            R_TABLE[i] = tmpRes;
            // T_i = ln(1/R_i) = -ln(R_i) = ln(c)
            T_TABLE[i] = static_cast<float>(-log_newton(static_cast<double>(tmpRes)));
        }
    }
    } // namespace detail

    extern "C" float logf (float x) {
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

        if (x == 1.0f) {
            return 0.0f;
        }

        if (x >= 0.85f && x <= 1.15f) {
            float f = x - 1.0f;
            // Больше всего вопросов к этой части: правильный ли подход и вообще можно ли так делать?
            // Полином до 8 степени по тейлору тк только при 9 степени получается ошибка меньше машинного эпсилон
            // Работает около нуля, тут ряд тейлора корректен
            float p = std::fmaf(f, -0.12500000f, 0.14285714f);
            p = std::fmaf(f, p, -0.16666667f);
            p = std::fmaf(f, p, 0.20000000f);
            p = std::fmaf(f, p, -0.25000000f);
            p = std::fmaf(f, p, 0.33333333f);
            p = std::fmaf(f, p, -0.50000000f);

            return std::fmaf(f * f, p, f);
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
        // std::cerr << std::setprecision(9) << "R_table[0] =" << R_TABLE[0] << "\nT_table[0]= " << T_TABLE[0] << "\n";

        // float r = Ri * x_0 - 1.0f; // <---CATASTROPHIC CANCELLATION WAS HERE
        float r = std::fmaf(Ri, x_0, -1.0f); // is this solution good enough?
        float poly = r * (POLY_1 + r * (POLY_2 + r * POLY_3));
        // float polyTemp = std::fmaf(r, POLY_3, POLY_2);
        // float poly = std::fmaf(r, polyTemp, POLY_1);
        // poly = r * poly;

        float res = poly + Ti; // divide result sum
        res = res + static_cast<float>(n) * LOG_2;

        return res;
    }
} // namespace math