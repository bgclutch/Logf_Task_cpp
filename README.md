# Logf task

A branchless, vectorized implementation of `logf()` in C++ (AVX2 / AVX-512),
built from scratch and benchmarked against `std::log`, SLEEF, and Intel MKL.

**Result:** 2.6 cycles/element throughput at 1.43 ULP max error —
7x faster than `std::log` and 2.2x faster than SLEEF.

## Test setup

- **CPU:** Intel Core i7-1260P @ 2.1 GHz
- **Compiler:** g++ (GCC) 13.4.0
- **Flags:** `-O3 -mavx2 -mfma`
- **Metric:** CPE (cycles per element), reported as mean ± standard deviation
- **Benchmark:** custom microbenchmark using `__rdtscp` for serialized timing
  and `_mm_lfence` as a hardware barrier. Latency and throughput are measured
  separately, since they stress different parts of the pipeline.

## Results

| Implementation      | Latency (CPE)   | Throughput (CPE) |
|----------------------|-----------------|-------------------|
| `std::log`           | 79.70 ± 1.49    | 18.27 ± 1.05      |
| `mylogf` (scalar)     | 55.24 ± 0.62    | 27.17 ± 0.93      |
| `mylogf_vec` (AVX2)   | —               | 2.62 ± 0.30       |
| SLEEF                 | —               | 5.68 ± 0.35       |
| Intel MKL HA          | —               | 2.05 ± 0.36       |
| Intel MKL LA          | —               | 1.78 ± 0.19       |

Max error: **1.43 ULP** over the normalized range (down from ~9000 ULP in the
first version — see below).

## How it evolved

Four iterations, each one fixing a problem introduced by the previous one.

### V1 — Naive range reduction to [1, 2)

Standard IEEE-754 decomposition: `x = 2^n * m`. The mantissa `m` was extracted
with a bit mask, `(ix & 0x007FFFFF) | 0x3f800000`, landing in `[1, 2)`.

**Problem:** the interval is too wide, so a long polynomial was needed for
acceptable accuracy. Worse, for `x` close to `1.0`, subtracting `1` caused
catastrophic cancellation — error reached **~9000 ULP**.

`Latency: 84.27 ± 16.33 CPE · Throughput: 78.41 ± 1.54 CPE`

### V2 — Branch + local Taylor series for [0.85, 1.15]

To fix the accuracy problem near `1.0`, I added a branch
(`if x >= 0.85f && x <= 1.15f`) that switched to a direct 8th-degree Taylor
expansion of `ln(1 + f)` on that interval.

**Problem:** this introduced two new issues:
1. An 8th-degree polynomial needs a lot of arithmetic.
2. The branch broke the CPU's branch predictor. On random input, the pipeline
   kept flushing, adding a real cost per misprediction[^1] — and branches also
   make the algorithm impossible to vectorize efficiently.

`Latency: 91.58 ± 3.40 CPE · Throughput: 90.76 ± 1.99 CPE`

### V3 — Branchless, shifted interval [2/3, 4/3], truncated tables

Branches removed entirely. A magic-constant subtraction,
`ux_norm = ux_bit - 0x3f2a2000u`, shifts the mantissa interval to `[2/3, 4/3)`,
putting `1.0` exactly in the middle.

- The exponent is now computed unconditionally via an arithmetic right shift
  (`>> 23`), which naturally borrows a bit for values below `2/3`.
- Table entries in `R_TABLE` have their low 11 mantissa bits zeroed
  (`& 0xfffff800U`), which makes `x_norm * R` exact in hardware and removes
  the cancellation problem for good — error drops to **1.43 ULP**.

Trade-off: the lookup-table generation had to be rewritten for the new interval.

`Latency: 55.24 ± 0.62 CPE · Throughput: 27.17 ± 0.93 CPE`

### V4 — Vectorization

Ported to SIMD. A set of macros (`VEC_ADD`, `VEC_GATHER`, `VEC_CMP`, etc.) let
a single source file compile for both AVX2 (8 elements/register) and AVX-512
(16 elements/register). Table lookups use hardware gather instructions.

`Throughput: 2.59 ± 0.26 CPE (AVX2)`

## Analysis

**Scalar vs. `std::log`:** the scalar version beats `std::log` on latency by
about 30%, mainly because it skips most of the `errno`-handling that `math.h`
does, and runs a branch-free pipeline. On throughput, though, the table
lookups become a bottleneck and the scalar version loses to `std::log`'s
pure-arithmetic approach.

**Vectorization removes that bottleneck:** throughput improves almost 10x over
scalar, reaching 2.59 CPE. Gather instructions are the dominant cost at this
point — I haven't yet measured exactly where the ceiling is on this CPU.[^2]

**vs. SLEEF (2.2x faster):** SLEEF favors low memory usage — high-degree
polynomials with no large tables. Keeping `R_TABLE`/`T_TABLE` in L1d instead
cuts the number of arithmetic ops in the hot loop, which is why this
implementation pulls ahead on throughput.

**vs. Intel MKL:** MKL High Accuracy is still meaningfully faster (2.05 vs.
2.62 CPE, roughly 28%)[^3] — a reasonable outcome given MKL is closed-source
and hand-tuned per Intel microarchitecture. My best guess is the gap comes
from gather scheduling and possibly table layout; I'd want to profile this
further before saying more.

## Trade-off: exceptions and denormals

Handling denormals inline (shifts and multiplications by `2^23`) would add
10–15 extra instructions to *every* element in the vector path — not
acceptable for a hot loop that's fast 99.9% of the time.

Instead, I used a **scalar fallback**: the vector fast path stays completely
branch-free, and a vector mask flags anything unusual:
- `x < FLT_MIN` (`0x00800000`) catches denormals, zero, and negative numbers
- `|x| >= Inf` (`0x7f800000`) catches infinities and NaN

If the mask is all-zero (the common case), the loop finishes at 2.59 CPE.
Otherwise, only the flagged (rare) elements are recomputed with the scalar
path (2.72 CPE) — zero overhead on clean data, at the cost of a slower path
for edge cases.

## Limitations and future work

- MKL HA is still ~28% faster in throughput; I suspect gather scheduling
  and/or table layout, but haven't profiled this in detail yet.
- Benchmarks ran on a hybrid P-core/E-core CPU (Alder Lake); I did not pin the
  benchmark thread to a specific core type, which could add noise to the
  reported numbers.
- Next: profile with `perf`/VTune to find the actual bottleneck vs. MKL, try
  pinning to a P-core, and add an AVX-512 benchmark run (code already
  supports it, only tested on AVX2 so far).

## Build and run

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/bench
```

[^1]: Branch misprediction penalty depends on pipeline depth and microarchitecture;
    on modern Intel cores it's typically in the 15–20 cycle range.
[^2]: Original analysis called this throughput "the physical limit of the
    memory port bandwidth for gather instructions" — worth re-checking, since
    MKL LA reaches 1.78 CPE on the same hardware, which suggests the ceiling
    is lower than 2.59 CPE and something else is the limiting factor here.
[^3]: Original analysis attributed this gap to instruction scheduler and
    gather latency "margin of error" — but 28% is a real, repeatable gap
    given the ~0.3 CPE standard deviations, not noise.
