/**
 * @file gyro.cpp
 * @author Xhovani Mali (xxm202)
 * @brief Implementation of gyroscope functionality in the embedded sentry project.
 * @version 0.1
 * @date 2024-12-15
 *
 *
 * @group Members:
 * - Xhovani Mali
 * - Shruti Pangare
 * - Temira Koenig
 */

#include "gyro.h"

SPI gyroscope(PF_9, PF_8, PF_7); // mosi, miso, sclk
DigitalOut cs(PC_1);

int16_t x_threshold; // X-axis calibration threshold
int16_t y_threshold; // Y-axis calibration threshold
int16_t z_threshold; // Z-axis calibration threshold

int16_t x_sample; // X-axis zero-rate level sample
int16_t y_sample; // Y-axis zero-rate level sample
int16_t z_sample; // Z-axis zero-rate level sample

float sensitivity = 0.0f;

Gyroscope_RawData *gyro_raw;

// Write I/O
void WriteByte(uint8_t address, uint8_t data)
{
    cs = 0;
    gyroscope.write(address);
    gyroscope.write(data);
    cs = 1;
}

// Get raw data from gyroscope
void GetGyroValue(Gyroscope_RawData *rawdata)
{
    cs = 0;
    gyroscope.write(OUT_X_L | 0x80 | 0x40); // auto-incremented read
    rawdata->x_raw = gyroscope.write(0xff) | gyroscope.write(0xff) << 8;
    rawdata->y_raw = gyroscope.write(0xff) | gyroscope.write(0xff) << 8;
    rawdata->z_raw = gyroscope.write(0xff) | gyroscope.write(0xff) << 8;
    cs = 1;
}

// Calibrate gyroscope before recording.
// Samples 128 readings to determine the zero-rate level (bias) and peak noise
// threshold for each axis. Data below the threshold is treated as zero to
// suppress ambient vibration.
void CalibrateGyroscope(Gyroscope_RawData *rawdata)
{
    int16_t sumX = 0;
    int16_t sumY = 0;
    int16_t sumZ = 0;
    for (int i = 0; i < 128; i++)
    {
        GetGyroValue(rawdata);
        sumX += rawdata->x_raw;
        sumY += rawdata->y_raw;
        sumZ += rawdata->z_raw;
        x_threshold = max(x_threshold, rawdata->x_raw);
        y_threshold = max(y_threshold, rawdata->y_raw);
        z_threshold = max(z_threshold, rawdata->z_raw);
        wait_us(10000);
    }

    x_sample = sumX >> 7; // 128 is 2^7
    y_sample = sumY >> 7;
    z_sample = sumZ >> 7;
}

// Initiate gyroscope, set up control registers
void InitiateGyroscope(Gyroscope_Init_Parameters *init_parameters, Gyroscope_RawData *init_raw_data)
{
    gyro_raw = init_raw_data;
    cs = 1;
    gyroscope.format(8, 3);       // 8 bits per SPI frame; polarity 1, phase 0
    gyroscope.frequency(1000000); // 1 MHz clock (max: 10 MHz)

    WriteByte(CTRL_REG_1, init_parameters->conf1 | POWERON); // set ODR, bandwidth, enable all axes
    WriteByte(CTRL_REG_3, init_parameters->conf3);           // DRDY enable
    WriteByte(CTRL_REG_4, init_parameters->conf4);           // full-scale selection

    switch (init_parameters->conf4)
    {
        case FULL_SCALE_245:
            sensitivity = SENSITIVITY_245;
            break;

        case FULL_SCALE_500:
            sensitivity = SENSITIVITY_500;
            break;

        case FULL_SCALE_2000:
            sensitivity = SENSITIVITY_2000;
            break;

        case FULL_SCALE_2000_ALT:
            sensitivity = SENSITIVITY_2000;
            break;
    }

    CalibrateGyroscope(gyro_raw);
}

// Convert raw ADC value to degrees per second
float ConvertToDPS(int16_t axis_data)
{
    return axis_data * sensitivity;
}

// Apply bias offset and noise threshold, writing calibrated values back to gyro_raw
void GetCalibratedRawData()
{
    GetGyroValue(gyro_raw);

    // Subtract the zero-rate level bias
    gyro_raw->x_raw -= x_sample;
    gyro_raw->y_raw -= y_sample;
    gyro_raw->z_raw -= z_sample;

    // Zero out readings below the noise threshold
    if (abs(gyro_raw->x_raw) < abs(x_threshold))
        gyro_raw->x_raw = 0;
    if (abs(gyro_raw->y_raw) < abs(y_threshold))
        gyro_raw->y_raw = 0;
    if (abs(gyro_raw->z_raw) < abs(z_threshold))
        gyro_raw->z_raw = 0;
}

// Turn off the gyroscope
void PowerOff()
{
    WriteByte(CTRL_REG_1, 0x00);
}
