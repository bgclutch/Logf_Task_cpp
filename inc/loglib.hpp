#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>

namespace math {
const float POLY_1 = 0x1.fffffffff6666p-1f;
const float POLY_2 = -0x1.00006000349d3p-1f;
const float POLY_3 = 0x1.55561555cccd4p-2f;
const float LOG_2  = 0x1.62e42fefa39efp-1f;

inline float R_TABLE[256];
inline float T_TABLE[256];

namespace detail {
double log_newton(double);
void initLookUpTables();
} // namespace detail

extern "C" float logf(float x);
extern "C" void logf_avx(float*, float*, const size_t);
} // namespace math