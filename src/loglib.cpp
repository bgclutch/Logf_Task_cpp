#include "loglib.hpp"

#include <bit>
#include <cerrno>
#include <cfenv>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <immintrin.h>

#ifdef __AVX512F__
#define VEC_WIDTH 16
#define ALIGNMENT 64
#define VEC_TYPEd __m512
#define VEC_TYPEi __m512i
#define VEC_LOAD _mm512_load_ps
#define VEC_STORE _mm512_store_ps
#define VEC_SETd _mm512_set1_ps
#define VEC_SETi _mm512_set1_epi32
#define VEC_SUB _mm512_sub_epi32
#define VEC_AND _mm512_and_si512
#define VEC_FMA _mm512_fmadd_ps
#define VEC_ADD _mm512_add_ps
#define VEC_MUL _mm512_mul_ps
#define VEC_SHIFTai _mm512_srai_epi32
#define VEC_SHIFTli _mm512_srli_epi32
#define VEC_GATHER _mm512_i32gather_ps
#define VEC_MASK _mm512_fpclass_ps_mask
#define VEC_CASTd _mm512_cvtepi32_ps
#define VEC_MASK_TYPE __mmask16
#define VEC_BIT_CASTi _mm512_castps_si512
#define VEC_BIT_CASTd _mm512_castsi512_ps
#else
#define VEC_WIDTH 8
#define ALIGNMENT 32
#define VEC_TYPEd __m256
#define VEC_TYPEi __m256i
#define VEC_MASK_TYPE int
#define VEC_LOAD _mm256_loadu_ps
#define VEC_STORE _mm256_storeu_ps
#define VEC_SETd _mm256_set1_ps
#define VEC_SETi _mm256_set1_epi32
#define VEC_SUB _mm256_sub_epi32
#define VEC_ADD _mm256_add_ps
#define VEC_SHIFTai _mm256_srai_epi32
#define VEC_SHIFTli _mm256_srli_epi32
#define VEC_CASTd _mm256_cvtepi32_ps
#define VEC_GATHER _mm256_i32gather_ps
#define VEC_AND _mm256_and_si256
#define VEC_ORPS _mm256_or_ps
#define VEC_ANDPS _mm256_and_ps
#define VEC_FMA _mm256_fmadd_ps
#define VEC_MUL _mm256_mul_ps
#define VEC_BIT_CASTi _mm256_castps_si256
#define VEC_BIT_CASTd _mm256_castsi256_ps
#define VEC_CMP _mm256_cmp_ps
#define VEC_MOVE_MASK _mm256_movemask_ps
#define VEC_SET_ZERO _mm256_setzero_ps
#endif

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
  for (int i = 0; i != 20; ++i) {
    y = y - 1. + x * std::exp(-y);
  }
  return y;
}
#if 0
    void initLookUpTables() {
        for (int i = 0; i != 256; ++i) {
            double c = 1.0 + static_cast<double>(i) / 256.0;
            float tmpRes = static_cast<float>(1. / c);
            R_TABLE[i] = tmpRes;
            // T_i = ln(1/R_i) = -ln(R_i) = ln(c)
            T_TABLE[i] = static_cast<float>(-log_newton(static_cast<double>(tmpRes)));
        }
    }
#endif
void initLookUpTables() {
  const int N = 256;
  const int num0 = 341;
  const int den = 2 * N;

  for (int i = 0; i < N; i++) {
    double x = static_cast<double>(num0 + i) / static_cast<double>(den);
    double C = 1. / x;
    if (static_cast<float>(C) < 1.0f) {
      x = static_cast<double>(num0 + i + (i - (den - num0))) /
          (static_cast<double>(den));
      C = 1. / x;
    }

    float C_tmp = static_cast<float>(C);
    R_TABLE[i] = C_tmp;
    T_TABLE[i] = static_cast<float>(-log_newton(static_cast<double>(C_tmp)));
  }
}
} // namespace detail

extern "C" float logf(float x) {
  uint32_t ux_bit = std::bit_cast<uint32_t>(x);
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

#if 0
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
#endif

  // Denormal numbers (Bonus part)
  int n = 0;
  if (ux_bit < 0x00800000) {
    x *= 8388608.0f;
    std::memcpy(&ux_bit, &x, 4);
    n -= 23;
  }

  uint32_t ux_norm = ux_bit - 0x3f2a2000u;
  n += static_cast<int32_t>(ux_norm) >> 23;

  uint32_t ux_red = ux_bit - (ux_norm & 0xff800000u);
  float x_norm = std::bit_cast<float>(ux_red);

  // unsigned int mantissa = (ix & 0x007FFFFF) | 0x3F800000;
  // float x_0;
  // std::memcpy(&x_0, &mantissa, 4);

  unsigned int idx = (ux_norm & 0x007FFFFF) >> 15;
  float Ri = R_TABLE[idx];
  float Ti = T_TABLE[idx];
  // std::cerr << std::setprecision(9) << "R_table[0] =" << R_TABLE[0] <<
  // "\nT_table[0]= " << T_TABLE[0] << "\n";

  // float r = Ri * x_norm - 1.0f; // <---CATASTROPHIC CANCELLATION WAS HERE
  float r = std::fmaf(Ri, x_norm, -1.0f); // is this solution good enough?
  // float poly = r * (POLY_1 + r * (POLY_2 + r * POLY_3));
  float polyTemp = std::fmaf(r, POLY_3, POLY_2);
  float poly = std::fmaf(r, polyTemp, POLY_1);
  poly = r * poly;

  float res = poly + Ti; // divide result sum
  res = res + static_cast<float>(n) * LOG_2;

  return res;
}

extern "C" void logf_avx(float* data, float* out, const size_t size) {
    const VEC_TYPEi RED_CONST = VEC_SETi(0x3f2a2000u);
    const VEC_TYPEi EXP_MASK  = VEC_SETi(std::bit_cast<int32_t>(0xff800000u));
    const VEC_TYPEi INDEX_VEC = VEC_SETi(std::bit_cast<int32_t>(0x007FFFFFu));
    const VEC_TYPEd NEG_ONE   = VEC_SETd(-1.0f);

    const VEC_TYPEd POLYv1    = VEC_SETd(0x1.fffffffff6666p-1f);
    const VEC_TYPEd POLYv2    = VEC_SETd(-0x1.00006000349d3p-1f);
    const VEC_TYPEd POLYv3    = VEC_SETd(0x1.55561555cccd4p-2f);
    const VEC_TYPEd LOG2v     = VEC_SETd(0.69314718056f);

    #ifndef __AVX512F__
    const VEC_TYPEd VEC_ABS = VEC_BIT_CASTd(VEC_SETi(0x7fffffffu));
    const VEC_TYPEd VEC_INF = VEC_BIT_CASTd(VEC_SETi(0x7f800000u));
    #endif


    size_t offset = 0;

    for (; (offset + VEC_WIDTH) <= size; offset += VEC_WIDTH) {
        VEC_TYPEd vec_x = VEC_LOAD(data + offset);

        #ifdef __AVX512F__
            VEC_MASK_TYPE exception_mask = VEC_MASK(vec_x, 0x7F);
        #else
            #if 1
            VEC_TYPEd bad_mask = VEC_ORPS(
                VEC_CMP(vec_x, VEC_SET_ZERO(), _CMP_LE_OQ),
                VEC_CMP(VEC_ANDPS(vec_x, VEC_ABS), VEC_INF, _CMP_GE_OQ)
            );
            #endif
            #if 0
            const VEC_TYPEd V_MIN_NORMAL = VEC_BIT_CASTd(VEC_SETi(0x00800000u));

            VEC_TYPEd bad_mask = VEC_ORPS(
            VEC_CMP(vec_x, V_MIN_NORMAL, _CMP_LT_OQ),
            VEC_CMP(VEC_ANDPS(vec_x, VEC_ABS), VEC_INF, _CMP_GE_OQ)
            );

            #endif
            VEC_MASK_TYPE exception_mask = VEC_MOVE_MASK(bad_mask);
        #endif

        VEC_TYPEi bit_vec_x = VEC_BIT_CASTi(vec_x);
        VEC_TYPEi vec_x_norm = VEC_SUB(bit_vec_x, RED_CONST);

        VEC_TYPEi vec_norm = VEC_SHIFTai(vec_x_norm, 23);
        VEC_TYPEd vec_n = VEC_CASTd(vec_norm);

        VEC_TYPEi v_exp_part = VEC_AND(vec_x_norm, EXP_MASK);
        VEC_TYPEi v_ux_red   = VEC_SUB(bit_vec_x, v_exp_part);
        VEC_TYPEd v_x_norm   = VEC_BIT_CASTd(v_ux_red);

        VEC_TYPEi vec_idx = VEC_AND(vec_x_norm, INDEX_VEC);
        vec_idx = VEC_SHIFTli(vec_idx, 15);

        #ifdef __AVX512F__
            VEC_TYPEd vec_Ri = VEC_GATHER(vec_idx, R_TABLE, 4);
            VEC_TYPEd vec_Ti = VEC_GATHER(vec_idx, T_TABLE, 4);
        #else
            VEC_TYPEd vec_Ri = VEC_GATHER(R_TABLE, vec_idx, 4);
            VEC_TYPEd vec_Ti = VEC_GATHER(T_TABLE, vec_idx, 4);
        #endif

        VEC_TYPEd vec_r = VEC_FMA(vec_Ri, v_x_norm, NEG_ONE);

        VEC_TYPEd v_poly = VEC_FMA(vec_r, POLYv3, POLYv2);
        v_poly = VEC_FMA(vec_r, v_poly, POLYv1);
        v_poly = VEC_MUL(vec_r, v_poly);

        VEC_TYPEd v_res = VEC_ADD(v_poly, vec_Ti);
        v_res = VEC_FMA(vec_n, LOG2v, v_res);

        VEC_STORE(out + offset, v_res);

        if (exception_mask != 0) {
            for (size_t i = 0; i < VEC_WIDTH; ++i) {
                if ((exception_mask >> i) & 1) {
                    out[offset + i] = math::logf(data[offset + i]);
                }
            }
        }
    }

    for (; offset < size; ++offset) {
        out[offset] = math::logf(data[offset]);
    }
}
} // namespace math

