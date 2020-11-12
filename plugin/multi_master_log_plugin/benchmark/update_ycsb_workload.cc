#include <stdint.h>
#include <cmath>
#include <cstdint>
#include <cstdlib>

#include "benchlog.h"
#include "update_ycsb_workload.h"

namespace Update_YSCB_Workload {

::BenchLog::BenchLogRecord gen_benchlog(uint64_t n, double theta)
{

}

// The following algorithm comes from the paper:
//
// Quickly generating billion-record synthetic databases
// However, it seems there is a small bug..
// The original paper says zeta(theta, 2.0). But I guess it should be.
// zeta(2.0, theta).
double zeta(uint64_t n, double theta) {
  double sum = 0;
  for (uint64_t i = 1; i <= n; i++) sum += pow(1.0 / i, theta);
  return sum;
}

uint64_t zipf(uint64_t n, double theta, drand48_data & buffer) {
  double zeta_2_theta = zeta(2,theta);
  double alpha = 1 / (1 - theta);
  double zetan = zeta(n , theta);
  double eta = (1 - pow(2.0 / n, 1 - theta)) / (1 - zeta_2_theta / zetan);
  double u;
  drand48_r(&buffer, &u);
  double uz = u * zetan;
  if (uz < 1) return 1;
  if (uz < 1 + pow(0.5, theta)) return 2;
  return 1 + (uint64_t)(n * pow(eta * u - eta + 1, alpha));
}

}  // namespace Update_YSCB_Workload
