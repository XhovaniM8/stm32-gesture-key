/**
 * @file utilities.h
 * @author Xhovani Mali
 * @brief Header file for utility functions in the embedded sentry project.
 * @version 0.1
 * @date 2024-12-15
 *
 *
 * @group Members:
 * - Xhovani Mali
 * - Shruti Pangare
 * - Temira Koenig
 */

#ifndef UTILITIES_H
#define UTILITIES_H

#include "system_config.h"

#define WINDOW_SIZE 5  // Moving average filter window size

/**
 * @brief Calculate the Pearson correlation for each axis between two gesture vectors
 * @param vec1: the first vector (contains 3D data)
 * @param vec2: the second vector (contains 3D data)
 * @return per-axis correlation [x, y, z] in [-1, 1]
 */
array<float, 3> calculateCorrelationVectors(vector<array<float, 3>> &vec1,
                                            vector<array<float, 3>> &vec2);

/**
 * @brief Calculate the Pearson correlation coefficient between two vectors
 * @param a: the first vector
 * @param b: the second vector
 * @return correlation in [-1, 1], or NaN on error
 */
float correlation(const vector<float> &a, const vector<float> &b);

/**
 * @brief Trim leading and trailing near-zero samples from gyro data
 * @param data: the gyro data to trim
 */
void trim_gyro_data(vector<array<float, 3>> &data);

#endif  // UTILITIES_H
