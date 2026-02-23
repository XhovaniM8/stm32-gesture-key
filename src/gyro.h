/**
 * @file gyro.h
 * @author Xhovani Mali (xxm202)
 * @brief Header file for gyroscope functionality in the embedded sentry project.
 * @version 0.1
 * @date 2024-12-15
 *
 *
 * @group Members:
 * - Xhovani Mali
 * - Shruti Pangare
 * - Temira Koenig
 */

#include <mbed.h>

#include "system_config.h"

// Initialization parameters
typedef struct {
  uint8_t conf1;  // output data rate
  uint8_t conf3;  // interrupt configuration
  uint8_t conf4;  // full-scale selection
} Gyroscope_Init_Parameters;

// Raw data
typedef struct {
  int16_t x_raw;  // X-axis raw data
  int16_t y_raw;  // Y-axis raw data
  int16_t z_raw;  // Z-axis raw data
} Gyroscope_RawData;

// Write IO
void WriteByte(uint8_t address, uint8_t data);

// Read IO
void GetGyroValue(Gyroscope_RawData *rawdata);

// Gyroscope calibration
void CalibrateGyroscope(Gyroscope_RawData *rawdata);

// Gyroscope initialization
void InitiateGyroscope(Gyroscope_Init_Parameters *init_parameters,
                       Gyroscope_RawData *init_raw_data);

// Data conversion: raw -> degrees per second
float ConvertToDPS(int16_t rawdata);

// Get calibrated raw data
void GetCalibratedRawData();

// Turn off the gyroscope
void PowerOff();
