/**
 * @file main.cpp
 * @author Xhovani Mali (xxm202)
 * @brief Entry point to embedded systems final project on embedded sentry.
 * @version 0.1
 * @date 2024-12-15
 *
 *
 * @group Members:
 * - Xhovani Mali
 * - Shruti Pangare
 * - Temira Koenig
 */

#include <mbed.h>                     // Mbed core
#include <vector>                     // For vector usage
#include <array>                      // For array usage
#include "utilities.h"                // Utility functions
#include "gyro.h"                     // Gyroscope functions
#include "system_config.h"            // System configuration
#include "drivers/LCD_DISCO_F429ZI.h" // LCD driver
#include "drivers/TS_DISCO_F429ZI.h"  // Touch screen driver

InterruptIn gyroscope_interrupt(PA_2, PullDown);
InterruptIn user_command_button(USER_BUTTON, PullDown);

DigitalOut led_status_green(LED1);
DigitalOut led_status_red(LED2);

LCD_DISCO_F429ZI lcd; // LCD object
TS_DISCO_F429ZI ts;   // Touch screen object

EventFlags flags; // Event flags

Timer timer; // Timer

float movingAverageFilter(float new_value, std::array<float, WINDOW_SIZE>& buffer, size_t& index, float& sum);
void normalize(vector<array<float, 3>>& data);

// Global variables for moving average filter
float gyro_buffer_x[WINDOW_SIZE] = {0};  // Buffers for x, y, z data
float gyro_buffer_y[WINDOW_SIZE] = {0};
float gyro_buffer_z[WINDOW_SIZE] = {0};
size_t gyro_index_x = 0, gyro_index_y = 0, gyro_index_z = 0;  // Index for each buffer
float gyro_sum_x = 0, gyro_sum_y = 0, gyro_sum_z = 0;  // Sum for each axis

/*******************************************************************************
 * Function Prototypes of LCD and Touch Screen
 * ****************************************************************************/
void draw_button(int x, int y, int width, int height, const char *label);
bool is_touch_inside_button(int touch_x, int touch_y, int button_x, int button_y, int button_width, int button_height);

void gyroscope_thread();
void touch_screen_thread();

/*******************************************************************************
 * ISR Callback Functions
 * ****************************************************************************/
void button_press() // Callback function for button press
{
    flags.set(ERASE_FLAG);
}
void onGyroDataReady() // Gyroscope data ready ISR
{
    flags.set(DATA_READY_FLAG);
}

/*******************************************************************************
 * @brief Global Variables
 * ****************************************************************************/
vector<array<float, 3>> gesture_key;      // the gesture key
vector<array<float, 3>> unlocking_record; // the unlocking record

const int button1_x = 60;
const int button1_y = 80;
const int button1_width = 120;
const int button1_height = 50;
const char *button1_label = "RECORD";
const int button2_x = 60;
const int button2_y = 180;
const int button2_width = 120;
const int button2_height = 50;
const char *button2_label = "UNLOCK";
const int message_x = 5;
const int message_y = 30;
const char *message = "EMBEDDED SENTRY";
const int text_x = 5;
const int text_y = 270;
const char *text_0 = "NO KEY RECORDED";
const char *text_1 = "LOCKED";

/*******************************************************************************
 * @brief main function
 * ****************************************************************************/
int main()
{
    lcd.Clear(LCD_COLOR_BLACK);

    // Draw button 1
    draw_button(button1_x, button1_y, button1_width, button1_height, button1_label);

    // Draw button 2
    draw_button(button2_x, button2_y, button2_width, button2_height, button2_label);

    // Display the welcome message
    lcd.DisplayStringAt(message_x, message_y, (uint8_t *)message, CENTER_MODE);

    // initialize all interrupts
    user_command_button.rise(&button_press);
    gyroscope_interrupt.rise(&onGyroDataReady);

    if (gesture_key.empty())
    {
        led_status_red = 0;
        led_status_green = 1;  // Green LED indicates ready to record
        lcd.SetTextColor(LCD_COLOR_GREEN);
        lcd.DisplayStringAt(text_x, text_y, (uint8_t *)text_0, CENTER_MODE);
    }
    else
    {
        led_status_red = 1;    // Red LED indicates locked
        led_status_green = 0;
        lcd.SetTextColor(LCD_COLOR_RED);
        lcd.DisplayStringAt(text_x, text_y, (uint8_t *)text_1, CENTER_MODE);
    }

    // Create the gyroscope thread
    Thread key_saving;
    key_saving.start(callback(gyroscope_thread));

    // Create the touch screen thread
    Thread touch_thread;
    touch_thread.start(callback(touch_screen_thread));

    // keep main thread alive
    while (1)
    {
        ThisThread::sleep_for(100ms);
    }
}

/*******************************************************************************
 * @brief Gyroscope Gesture Key Saving Thread
 *
 * This thread handles the initialization, recording, saving, and unlocking of
 * gesture keys using the gyroscope. It also handles the LCD display updates and
 * LED status indicators during the process.
 *
 * ****************************************************************************/
void gyroscope_thread()
{
    // Initialize gyroscope configuration parameters
    Gyroscope_Init_Parameters init_parameters = {
            ODR_200_CUTOFF_50, // Output data rate
            INT2_DRDY,         // Interrupt configuration
            FULL_SCALE_500     // Full-scale selection
    };

    // Set up gyroscope's raw data
    Gyroscope_RawData raw_data;
    char display_buffer[50];

    // Ensure the data-ready flag is set if the gyroscope interrupt is triggered
    if (!(flags.get() & DATA_READY_FLAG) && (gyroscope_interrupt.read() == 1))
    {
        flags.set(DATA_READY_FLAG);
    }

    while (1)
    {
        vector<array<float, 3>> temp_key; // Temporary key to store recorded gyroscope data

        // Wait for a flag indicating recording, unlocking, or erasing actions
        auto flag_check = flags.wait_any(KEY_FLAG | UNLOCK_FLAG | ERASE_FLAG);

        // Handle key erasing action
        if (flag_check & ERASE_FLAG)
        {
            sprintf(display_buffer, "Erasing....");
            lcd.SetTextColor(LCD_COLOR_BLACK);
            lcd.FillRect(0, text_y, lcd.GetXSize(), FONT_SIZE);
            lcd.SetTextColor(LCD_COLOR_YELLOW);
            lcd.DisplayStringAt(text_x, text_y, (uint8_t *)display_buffer, CENTER_MODE);

            // Clear gesture key and unlocking record
            gesture_key.clear();
            unlocking_record.clear();

            sprintf(display_buffer, "Key Erased.");
            lcd.SetTextColor(LCD_COLOR_BLACK);
            lcd.FillRect(0, text_y, lcd.GetXSize(), FONT_SIZE);
            lcd.SetTextColor(LCD_COLOR_YELLOW);
            lcd.DisplayStringAt(text_x, text_y, (uint8_t *)display_buffer, CENTER_MODE);

            led_status_green = 1;
            led_status_red = 0;
        }

        // Handle key recording or unlocking actions
        if (flag_check & (KEY_FLAG | UNLOCK_FLAG))
        {
            sprintf(display_buffer, "Hold On");
            lcd.SetTextColor(LCD_COLOR_BLACK);
            lcd.FillRect(0, text_y, lcd.GetXSize(), FONT_SIZE);
            lcd.SetTextColor(LCD_COLOR_ORANGE);
            lcd.DisplayStringAt(text_x, text_y, (uint8_t *)display_buffer, CENTER_MODE);

            ThisThread::sleep_for(1s);

            // Calibrate gyroscope
            sprintf(display_buffer, "Calibrating...");
            lcd.SetTextColor(LCD_COLOR_BLACK);
            lcd.FillRect(0, text_y, lcd.GetXSize(), FONT_SIZE);
            lcd.SetTextColor(LCD_COLOR_LIGHTGRAY);
            lcd.DisplayStringAt(text_x, text_y, (uint8_t *)display_buffer, CENTER_MODE);

            // Initialize the gyroscope
            InitiateGyroscope(&init_parameters, &raw_data);

            // Start recording gesture with countdown
            for (int i = 3; i > 0; --i)
            {
                sprintf(display_buffer, "Recording in %d...", i);
                lcd.SetTextColor(LCD_COLOR_BLACK);
                lcd.FillRect(0, text_y, lcd.GetXSize(), FONT_SIZE);
                lcd.SetTextColor(LCD_COLOR_ORANGE);
                lcd.DisplayStringAt(text_x, text_y, (uint8_t *)display_buffer, CENTER_MODE);
                ThisThread::sleep_for(1s);
            }

            sprintf(display_buffer, "Recording...");
            lcd.SetTextColor(LCD_COLOR_BLACK);
            lcd.FillRect(0, text_y, lcd.GetXSize(), FONT_SIZE);
            lcd.SetTextColor(LCD_COLOR_GREEN);
            lcd.DisplayStringAt(text_x, text_y, (uint8_t *)display_buffer, CENTER_MODE);

            // Gyro data recording loop (3 seconds at 20 Hz)
            timer.start();
            while (timer.elapsed_time() < 3s)
            {
                flags.wait_all(DATA_READY_FLAG);
                GetCalibratedRawData();

                // Apply the moving average filter to smooth gyroscope data
                float smoothed_x = movingAverageFilter(ConvertToDPS(raw_data.x_raw),
                                                 reinterpret_cast<array<float, 5> &>(gyro_buffer_x), gyro_index_x, gyro_sum_x);
                float smoothed_y = movingAverageFilter(ConvertToDPS(raw_data.y_raw),
                                                 reinterpret_cast<array<float, 5> &>(gyro_buffer_y), gyro_index_y, gyro_sum_y);
                float smoothed_z = movingAverageFilter(ConvertToDPS(raw_data.z_raw),
                                                 reinterpret_cast<array<float, 5> &>(gyro_buffer_z), gyro_index_z, gyro_sum_z);

                temp_key.push_back({smoothed_x, smoothed_y, smoothed_z});
                ThisThread::sleep_for(50ms); // 20 Hz sampling
            }
            timer.stop();
            timer.reset();

            // Trim leading/trailing zero data
            trim_gyro_data(temp_key);

            sprintf(display_buffer, "Finished...");
            lcd.SetTextColor(LCD_COLOR_BLACK);
            lcd.FillRect(0, text_y, lcd.GetXSize(), FONT_SIZE);
            lcd.SetTextColor(LCD_COLOR_GREEN);
            lcd.DisplayStringAt(text_x, text_y, (uint8_t *)display_buffer, CENTER_MODE);
        }

        // Handle saving or replacing gesture keys
        if (flag_check & KEY_FLAG)
        {
            if (gesture_key.empty())
            {
                sprintf(display_buffer, "Saving Key...");
                lcd.SetTextColor(LCD_COLOR_BLACK);
                lcd.FillRect(0, text_y, lcd.GetXSize(), FONT_SIZE);
                lcd.SetTextColor(LCD_COLOR_LIGHTGREEN);
                lcd.DisplayStringAt(text_x, text_y, (uint8_t *)display_buffer, CENTER_MODE);

                gesture_key = temp_key;
                temp_key.clear();

                led_status_red = 1;
                led_status_green = 0;

                sprintf(display_buffer, "Key saved.");
                lcd.SetTextColor(LCD_COLOR_BLACK);
                lcd.FillRect(0, text_y, lcd.GetXSize(), FONT_SIZE);
                lcd.SetTextColor(LCD_COLOR_LIGHTGREEN);
                lcd.DisplayStringAt(text_x, text_y, (uint8_t *)display_buffer, CENTER_MODE);
            }
            else
            {
                sprintf(display_buffer, "Removing old key...");
                lcd.SetTextColor(LCD_COLOR_BLACK);
                lcd.FillRect(0, text_y, lcd.GetXSize(), FONT_SIZE);
                lcd.SetTextColor(LCD_COLOR_ORANGE);
                lcd.DisplayStringAt(text_x, text_y, (uint8_t *)display_buffer, CENTER_MODE);

                ThisThread::sleep_for(1s);

                gesture_key.clear();
                gesture_key = temp_key;
                temp_key.clear();

                sprintf(display_buffer, "New key saved.");
                lcd.SetTextColor(LCD_COLOR_BLACK);
                lcd.FillRect(0, text_y, lcd.GetXSize(), FONT_SIZE);
                lcd.SetTextColor(LCD_COLOR_LIGHTGREEN);
                lcd.DisplayStringAt(text_x, text_y, (uint8_t *)display_buffer, CENTER_MODE);

                led_status_red = 1;
                led_status_green = 0;
            }
        }
        else if (flag_check & UNLOCK_FLAG)
        {
            flags.clear(UNLOCK_FLAG);
            sprintf(display_buffer, "Unlocking...");
            lcd.SetTextColor(LCD_COLOR_BLACK);
            lcd.FillRect(0, text_y, lcd.GetXSize(), FONT_SIZE);
            lcd.SetTextColor(LCD_COLOR_LIGHTGRAY);
            lcd.DisplayStringAt(text_x, text_y, (uint8_t *)display_buffer, CENTER_MODE);

            unlocking_record = temp_key;
            temp_key.clear();

            if (gesture_key.empty())
            {
                sprintf(display_buffer, "NO KEY SAVED.");
                lcd.SetTextColor(LCD_COLOR_BLACK);
                lcd.FillRect(0, text_y, lcd.GetXSize(), FONT_SIZE);
                lcd.SetTextColor(LCD_COLOR_RED);
                lcd.DisplayStringAt(text_x, text_y, (uint8_t *)display_buffer, CENTER_MODE);

                unlocking_record.clear();
                led_status_green = 1;
                led_status_red = 0;
            }
            else
            {
                int unlock_count = 0; // number of axes exceeding the correlation threshold

                // Align both recordings to the same length
                size_t target_size = std::min(gesture_key.size(), unlocking_record.size());
                gesture_key.resize(target_size);
                unlocking_record.resize(target_size);

                // Normalize both gesture and unlocking records
                normalize(gesture_key);
                normalize(unlocking_record);

                array<float, 3> correlationResult = calculateCorrelationVectors(gesture_key, unlocking_record);

                // Check if all three axes exceed the correlation threshold
                for (size_t i = 0; i < correlationResult.size(); i++)
                {
                    if (correlationResult[i] > CORRELATION_THRESHOLD)
                    {
                        unlock_count++;
                    }
                }

                // Update the display and LED status based on unlock result
                if (unlock_count == 3)  // All three axes must match
                {
                    sprintf(display_buffer, "UNLOCK: SUCCESS");
                    lcd.SetTextColor(LCD_COLOR_BLACK);
                    lcd.FillRect(0, text_y, lcd.GetXSize(), FONT_SIZE);
                    lcd.SetTextColor(LCD_COLOR_GREEN);
                    lcd.DisplayStringAt(text_x, text_y, (uint8_t *)display_buffer, CENTER_MODE);

                    led_status_green = 1;
                    led_status_red = 0;
                }
                else
                {
                    sprintf(display_buffer, "UNLOCK: FAILED");
                    lcd.SetTextColor(LCD_COLOR_BLACK);
                    lcd.FillRect(0, text_y, lcd.GetXSize(), FONT_SIZE);
                    lcd.SetTextColor(LCD_COLOR_RED);
                    lcd.DisplayStringAt(text_x, text_y, (uint8_t *)display_buffer, CENTER_MODE);

                    led_status_green = 0;
                    led_status_red = 1;
                }

                unlocking_record.clear();
            }
        }

        ThisThread::sleep_for(50ms);
    }
}

/*******************************************************************************
 *
 * @brief touch screen thread
 *
 * ****************************************************************************/
void touch_screen_thread()
{
    TS_StateTypeDef ts_state;

    if (ts.Init(lcd.GetXSize(), lcd.GetYSize()) != TS_OK)
    {
        return;
    }

    char display_buffer[50];

    while (1)
    {
        ts.GetState(&ts_state);
        if (ts_state.TouchDetected)
        {
            int touch_x = ts_state.X;
            int touch_y = ts_state.Y;

            // Check if the touch is inside record button
            if (is_touch_inside_button(touch_x, touch_y, button1_x, button1_y, button1_width, button1_height))
            {
                sprintf(display_buffer, "Recording Initiated...");
                lcd.SetTextColor(LCD_COLOR_BLACK);
                lcd.FillRect(0, text_y, lcd.GetXSize(), FONT_SIZE);
                lcd.SetTextColor(LCD_COLOR_BLUE);
                lcd.DisplayStringAt(text_x, text_y, (uint8_t *)display_buffer, CENTER_MODE);
                ThisThread::sleep_for(1s);
                flags.set(KEY_FLAG);
            }

            // Check if the touch is inside unlock button
            if (is_touch_inside_button(touch_x, touch_y, button2_x, button2_y, button2_width, button2_height))
            {
                sprintf(display_buffer, "Unlocking Initiated...");
                lcd.SetTextColor(LCD_COLOR_BLACK);
                lcd.FillRect(0, text_y, lcd.GetXSize(), FONT_SIZE);
                lcd.SetTextColor(LCD_COLOR_BLUE);
                lcd.DisplayStringAt(text_x, text_y, (uint8_t *)display_buffer, CENTER_MODE);
                ThisThread::sleep_for(1s);
                flags.set(UNLOCK_FLAG);
            }
        }
        ThisThread::sleep_for(10ms);
    }
}

/*******************************************************************************
 *
 * @brief draw button
 * @param x: x coordinate of the button
 * @param y: y coordinate of the button
 * @param width: width of the button
 * @param height: height of the button
 * @param label: label of the button
 *
 * ****************************************************************************/
void draw_button(int x, int y, int width, int height, const char *label)
{
    lcd.SetTextColor(LCD_COLOR_RED);
    lcd.FillRect(x, y, width, height);
    lcd.DisplayStringAt(x + width / 2 - strlen(label) * 19, y + height / 2 - 8, (uint8_t *)label, CENTER_MODE);
}

/*******************************************************************************
 *
 * @brief Check if the touch point is inside the button
 * @param touch_x: x coordinate of the touch point
 * @param touch_y: y coordinate of the touch point
 * @param button_x: x coordinate of the button
 * @param button_y: y coordinate of the button
 * @param button_width: width of the button
 * @param button_height: height of the button
 *
 * ****************************************************************************/
bool is_touch_inside_button(int touch_x, int touch_y, int button_x, int button_y, int button_width, int button_height)
{
    return (touch_x >= button_x && touch_x <= button_x + button_width &&
            touch_y >= button_y && touch_y <= button_y + button_height);
}

float movingAverageFilter(float new_value, std::array<float, WINDOW_SIZE>& buffer, size_t& index, float& sum) {
    sum -= buffer[index];
    buffer[index] = new_value;
    sum += new_value;
    index = (index + 1) % WINDOW_SIZE;
    return sum / WINDOW_SIZE;
}

void normalize(vector<array<float, 3>>& data) {
    for (auto& point : data) {
        float magnitude = sqrt(point[0] * point[0] + point[1] * point[1] + point[2] * point[2]);
        if (magnitude > 0) {
            point[0] /= magnitude;
            point[1] /= magnitude;
            point[2] /= magnitude;
        }
    }
}
