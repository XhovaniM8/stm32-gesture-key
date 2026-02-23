# stm32-gyro-auth

Gesture-based authentication system running on the **STM32F429ZI Discovery board**. Records a gyroscope motion sequence as a "key", then uses per-axis Pearson correlation to verify subsequent unlock attempts in real time.

## Hardware

| Component | Part |
|-----------|------|
| MCU | STM32F429ZI (ARM Cortex-M4 @ 180 MHz) |
| Gyroscope | L3GD20 3-axis (SPI, 500 dps full-scale) |
| Display | ILI9341 LCD 240×320 with STMPE811 touchscreen |
| Framework | Mbed OS |

## How It Works

1. **Record** — Press the RECORD button on the touchscreen. The system waits 1 s, calibrates the gyroscope (128-sample bias estimation), then records 3 s of motion at 20 Hz. A 5-sample moving average filter smooths each axis before the data is trimmed of leading/trailing silence.

2. **Unlock** — Press UNLOCK and repeat the gesture. The stored key and the new recording are normalized and compared via Pearson correlation on each axis independently. All three axes must exceed `CORRELATION_THRESHOLD` (default: 0.70) for the unlock to succeed.

3. **Erase** — Press the onboard user button (PA_0) at any time to clear the stored key.

## Project Structure

```
src/
├── main.cpp          – Application logic, threads, UI, gesture matching
├── gyro.cpp / .h     – L3GD20 SPI driver, calibration, DPS conversion
├── utilities.cpp / .h– Pearson correlation, moving average filter, data trimming
├── system_config.h   – Register definitions, sensitivity constants, event flags
├── serial_dump.py    – Python script for monitoring serial output during development
└── drivers/          – STM32 HAL, LCD, and touchscreen vendor drivers
```

## Build & Flash

Requires [PlatformIO](https://platformio.org/) with the ST STM32 platform installed.

```bash
# Build
pio run

# Flash to connected board
pio run --target upload
```

## Configuration

Edit `src/system_config.h` to tune system behaviour:

| Constant | Default | Description |
|----------|---------|-------------|
| `CORRELATION_THRESHOLD` | `0.70` | Minimum per-axis correlation to accept unlock (0.0–1.0) |
| `ODR_200_CUTOFF_50` | — | Gyroscope output data rate / cutoff |
| `FULL_SCALE_500` | — | Gyroscope full-scale range (±500 dps) |

## Authors

- Xhovani Mali
- Shruti Tulshidas Pangare
- Temira Koenig
