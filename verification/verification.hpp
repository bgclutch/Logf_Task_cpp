#include "loglib.hpp"
#include <cstdint>
#include <fstream>

namespace math {
namespace verification {
void run_interval_test(float, float, size_t, std::ofstream&, std::ofstream&);
float calc_ulp_error(double, float);
void check_val(float, std::ofstream&);
} // namespace verification
} // namespace math