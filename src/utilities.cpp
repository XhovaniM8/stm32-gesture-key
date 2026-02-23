/**
 * @file utilities.cpp
 * @author Xhovani Mali (xxm202)
 * @brief Utility functions implementation for the embedded sentry project.
 * @version 0.1
 * @date 2024-12-15
 *
 *
 * @group Members:
 * - Xhovani Mali
 * - Shruti Pangare
 * - Temira Koenig
 */

#include <array>
#include "utilities.h"

array<float, 3> calculateCorrelationVectors(vector<array<float, 3>> &vec1,
                                            vector<array<float, 3>> &vec2) {
  array<float, 3> result;

  // Ensure both vectors are of the same size
  if (vec1.size() != vec2.size()) {
    size_t min_size = std::min(vec1.size(), vec2.size());
    vec1.resize(min_size);
    vec2.resize(min_size);
  }

  size_t n = vec1.size();

  for (int i = 0; i < 3; i++) {
    vector<float> a, b;
    for (size_t j = 0; j < n; ++j) {
      a.push_back(vec1[j][i]);
      b.push_back(vec2[j][i]);
    }
    result[i] = correlation(a, b);
  }

  return result;
}

/*******************************************************************************
 *
 * @brief Calculate the Pearson correlation coefficient between two vectors
 * @param a: the first vector
 * @param b: the second vector
 * @return correlation in [-1, 1], or NaN on error
 *
 * ****************************************************************************/
float correlation(const vector<float> &a, const vector<float> &b) {
  if (a.size() != b.size()) {
    return std::numeric_limits<float>::quiet_NaN();
  }

  // Return NaN if all data is zero (no variation to correlate)
  bool has_variation = false;
  for (size_t i = 0; i < a.size(); ++i) {
    if (a[i] != 0.0f || b[i] != 0.0f) {
      has_variation = true;
      break;
    }
  }
  if (!has_variation) {
    return std::numeric_limits<float>::quiet_NaN();
  }

  float sum_a = 0, sum_b = 0, sum_ab = 0, sq_sum_a = 0, sq_sum_b = 0;

  // Kahan summation for numerical stability
  for (size_t i = 0; i < a.size(); ++i) {
    float delta_a = a[i] - sum_a;
    float delta_b = b[i] - sum_b;
    sum_a += delta_a;
    sum_b += delta_b;
    sum_ab += delta_a * delta_b;
    sq_sum_a += a[i] * a[i];
    sq_sum_b += b[i] * b[i];
  }

  size_t n = a.size();

  float numerator = sum_ab - (sum_a * sum_b / n);
  float denominator =
      sqrt((sq_sum_a - sum_a * sum_a / n) *
           (sq_sum_b - sum_b * sum_b / n));

  if (denominator == 0.0f) {
    return std::numeric_limits<float>::quiet_NaN();
  }

  return numerator / denominator;
}

/*******************************************************************************
 *
 * @brief Trim leading and trailing near-zero samples from gyro data
 * @param data: the gyro data to trim
 *
 * ****************************************************************************/
void trim_gyro_data(vector<array<float, 3>> &data) {
  float threshold = 0.00001;
  auto ptr = data.begin();

  // Find the first sample where any axis exceeds the threshold
  while (abs((*ptr)[0]) <= threshold && abs((*ptr)[1]) <= threshold &&
         abs((*ptr)[2]) <= threshold) {
    ptr++;
  }
  if (ptr == data.end()) return;  // all data below threshold

  auto lptr = ptr;  // left bound

  // Find the last sample where any axis exceeds the threshold
  ptr = data.end() - 1;
  while (abs((*ptr)[0]) <= threshold && abs((*ptr)[1]) <= threshold &&
         abs((*ptr)[2]) <= threshold) {
    ptr--;
  }
  auto rptr = ptr;  // right bound

  // Shift active region to the front
  auto replace_ptr = data.begin();
  for (; replace_ptr != lptr && lptr <= rptr; replace_ptr++, lptr++) {
    *replace_ptr = *lptr;
  }

  // Erase the tail
  if (lptr > rptr) {
    data.erase(replace_ptr, data.end());
  } else {
    data.erase(rptr + 1, data.end());
  }
}
