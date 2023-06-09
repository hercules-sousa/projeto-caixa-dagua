#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_system.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <esp_err.h>
#include <ds18x20.h>
#include <inttypes.h>
#include <sys/time.h>
#include <hd44780.h>
#include <pcf8574.h>
#include <string.h>
#include <math.h>

#define TRIGGER_PIN GPIO_NUM_1
#define ECHO_PIN GPIO_NUM_0
#define YELLOW_BOMB_LED_PIN GPIO_NUM_8
#define GREEN_RESISTENCE_LED_PIN GPIO_NUM_12
#define BUTTON_PIN1 GPIO_NUM_7
#define BUTTON_PIN2 GPIO_NUM_6
#define BUTTON_PIN3 GPIO_NUM_3
#define BUTTON_PIN4 GPIO_NUM_10
#define MAX_TANK_LEVEL 20
#define DISTANCE_BETWEEN_SENSOR_TANK 3.5
#define MAX_SENSORS 1
#define TANK_MIN 23.74
#define TANK_MAX 7.74
#define MIN_DISTANCE 10
#define MAX_DISTANCE 100
#define LCD_STRING_SIZE 16

static const gpio_num_t SENSOR_GPIO = 13;
static const int RESCAN_INTERVAL = 8;
static const uint32_t LOOP_DELAY_MS = 100;
static int is_choose_config_menu_on = 0;
static int is_config_menu_on = 0;
static int distance_percentage = 50;
static int temperature_level = 30;
static int is_bomb_on = 0;
static int is_resistance_on = 0;

static const char *TAG = "Project";

static i2c_dev_t pcf8574;

static const uint8_t char_data[] = {
    0x04, 0x0e, 0x0e, 0x0e, 0x1f, 0x00, 0x04, 0x00,
    0x1f, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0x1f, 0x00};

static esp_err_t write_lcd_data(const hd44780_t *lcd, uint8_t data)
{
    return pcf8574_port_write(&pcf8574, data);
}

hd44780_t lcd = {
    .write_cb = write_lcd_data,
    .font = HD44780_FONT_5X8,
    .lines = 2,
    .pins = {
        .rs = 0,
        .e = 2,
        .d4 = 4,
        .d5 = 5,
        .d6 = 6,
        .d7 = 7,
        .bl = 3}};

volatile int64_t pulse_start;
volatile int64_t pulse_end;

void write_on_lcd(char *string, int line)
{
    hd44780_gotoxy(&lcd, 0, line);
    hd44780_puts(&lcd, string);
}

void IRAM_ATTR echo_isr_handler(void *arg)
{
    if (gpio_get_level(ECHO_PIN))
    {
        pulse_start = esp_timer_get_time();
    }
    else
    {
        pulse_end = esp_timer_get_time();
    }
}

float calculate_percent_distance()
{
    uint32_t pulse_duration = pulse_end - pulse_start;
    float distance = pulse_duration / 58.0;

    int percent = ((distance - TANK_MIN) / (TANK_MAX - TANK_MIN)) * 100;

    ESP_LOGI(TAG, "Percent = %d", percent);

    return percent;
}

void distance_task(void *pvParameter)
{

    char string_distance[LCD_STRING_SIZE];
    int counter_level_above_set = 0;
    int counter_level_below_set = 0;
    int biggest_string_size = 0;

    while (1)
    {
        // Trigger pulse
        gpio_set_level(TRIGGER_PIN, 1);
        ets_delay_us(10);
        gpio_set_level(TRIGGER_PIN, 0);

        vTaskDelay(100 / portTICK_PERIOD_MS);

        float distance = calculate_percent_distance();

        if (!is_bomb_on)
        {
            if (distance <= distance_percentage)
            {
                counter_level_below_set++;
                if (counter_level_below_set >= 3)
                {
                    gpio_set_level(YELLOW_BOMB_LED_PIN, 1);
                    is_bomb_on = 1;
                }
            }
            else
            {
                counter_level_below_set = 0;
            }
        }
        else
        {
            if (distance > distance_percentage)
            {
                counter_level_above_set++;
                if (counter_level_above_set >= 3)
                {
                    gpio_set_level(YELLOW_BOMB_LED_PIN, 0);
                    is_bomb_on = 0;
                }
            }
            else
            {
                counter_level_above_set = 0;
            }
        }

        snprintf(string_distance, sizeof(string_distance), "Dist = %d%%     ", (int)distance);

        biggest_string_size = strlen(string_distance);

        if (!is_choose_config_menu_on && !is_config_menu_on)
        {
            write_on_lcd(string_distance, 0);
        }

        // ESP_LOGI(TAG, "Distance = %.2f", distance);
    }
}

void hcsr04_setup()
{
    // Configurar os pinos como GPIO de saída e entrada, respectivamente
    gpio_pad_select_gpio(TRIGGER_PIN);
    gpio_set_direction(TRIGGER_PIN, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(ECHO_PIN);
    gpio_set_direction(ECHO_PIN, GPIO_MODE_INPUT);

    gpio_set_intr_type(ECHO_PIN, GPIO_INTR_ANYEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(ECHO_PIN, echo_isr_handler, NULL);
}

void temperature_task(void *pvParameter)
{

    ds18x20_addr_t addrs[MAX_SENSORS];
    float temps[MAX_SENSORS];
    size_t sensor_count = 0;

    gpio_set_pull_mode(SENSOR_GPIO, GPIO_PULLUP_ONLY);

    char stringTemperature[LCD_STRING_SIZE];

    memset(stringTemperature, ' ', LCD_STRING_SIZE);
    stringTemperature[LCD_STRING_SIZE] = '\0';

    int counter_temperature_above_set = 0;
    int counter_temperature_below_set = 0;

    esp_err_t res;
    while (1)
    {
        // Every RESCAN_INTERVAL samples, check to see if the sensors connected
        // to our bus have changed.
        res = ds18x20_scan_devices(SENSOR_GPIO, addrs, MAX_SENSORS, &sensor_count);
        if (res != ESP_OK)
        {
            ESP_LOGE(TAG, "Sensors scan error %d (%s)", res, esp_err_to_name(res));
            continue;
        }

        if (!sensor_count)
        {
            ESP_LOGW(TAG, "No sensors detected!");
            continue;
        }

        ESP_LOGI(TAG, "%d sensors detected", sensor_count);

        // If there were more sensors found than we have space to handle,
        // just report the first MAX_SENSORS..
        if (sensor_count > MAX_SENSORS)
            sensor_count = MAX_SENSORS;

        // Do a number of temperature samples, and print the results.
        for (int i = 0; i < RESCAN_INTERVAL; i++)
        {
            // ESP_LOGI(TAG, "Measuring...");

            res = ds18x20_measure_and_read_multi(SENSOR_GPIO, addrs, sensor_count, temps);
            if (res != ESP_OK)
            {
                ESP_LOGE(TAG, "Sensors read error %d (%s)", res, esp_err_to_name(res));
                continue;
            }

            for (int j = 0; j < sensor_count; j++)
            {
                float temp_c = temps[j];

                snprintf(stringTemperature, sizeof(stringTemperature), "Temp = %.2f\xDF\C  ", temp_c);

                if (!is_choose_config_menu_on && !is_config_menu_on)
                {
                    write_on_lcd(stringTemperature, 1);
                }

                if (!is_resistance_on)
                {
                    if (temp_c <= temperature_level)
                    {
                        counter_temperature_below_set++;
                        if (counter_temperature_below_set >= 3 && distance_percentage >= 10)
                        {
                            gpio_set_level(GREEN_RESISTENCE_LED_PIN, 1);
                            is_resistance_on = 1;
                        }
                    }
                    else
                    {
                        counter_temperature_below_set = 0;
                    }
                }
                else
                {
                    if (temp_c > temperature_level)
                    {
                        counter_temperature_above_set++;
                        if (counter_temperature_above_set >= 3)
                        {
                            gpio_set_level(GREEN_RESISTENCE_LED_PIN, 0);
                            is_resistance_on = 0;
                        }
                    }
                    else
                    {
                        counter_temperature_above_set = 0;
                    }
                }

                ESP_LOGI(TAG, "Temperature = %.3f°C", temp_c);
            }

            vTaskDelay(pdMS_TO_TICKS(LOOP_DELAY_MS));
        }
    }
}

void led_setup()
{
    gpio_pad_select_gpio(YELLOW_BOMB_LED_PIN);
    gpio_set_direction(YELLOW_BOMB_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(GREEN_RESISTENCE_LED_PIN);
    gpio_set_direction(GREEN_RESISTENCE_LED_PIN, GPIO_MODE_OUTPUT);

    gpio_set_level(GREEN_RESISTENCE_LED_PIN, 0);
}

void lcd_setup()
{
    ESP_ERROR_CHECK(i2cdev_init());

    memset(&pcf8574, 0, sizeof(i2c_dev_t));
    ESP_ERROR_CHECK(pcf8574_init_desc(&pcf8574, 0x27, 0, 4, 5));

    ESP_ERROR_CHECK(hd44780_init(&lcd));

    hd44780_switch_backlight(&lcd, true);

    hd44780_upload_character(&lcd, 0, char_data);
    hd44780_upload_character(&lcd, 1, char_data + 8);
}

void buttons_setup()
{
    gpio_pad_select_gpio(BUTTON_PIN1);
    gpio_set_direction(BUTTON_PIN1, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PIN1, GPIO_PULLDOWN_ONLY);

    gpio_pad_select_gpio(BUTTON_PIN2);
    gpio_set_direction(BUTTON_PIN2, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PIN2, GPIO_PULLDOWN_ONLY);

    gpio_pad_select_gpio(BUTTON_PIN3);
    gpio_set_direction(BUTTON_PIN3, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PIN3, GPIO_PULLDOWN_ONLY);

    gpio_pad_select_gpio(BUTTON_PIN3);
    gpio_set_direction(BUTTON_PIN3, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PIN3, GPIO_PULLDOWN_ONLY);

    gpio_pad_select_gpio(BUTTON_PIN4);
    gpio_set_direction(BUTTON_PIN4, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PIN4, GPIO_PULLDOWN_ONLY);
}

void app_main()
{
    hcsr04_setup();
    led_setup();
    lcd_setup();
    buttons_setup();

    xTaskCreate(distance_task, "distance_task", 2048, NULL, 5, NULL);
    xTaskCreate(temperature_task, "temperature_task", configMINIMAL_STACK_SIZE * 32, NULL, 5, NULL);

    int state1 = 1;
    int previous_state1 = state1;

    int state2 = 1;
    int previous_state2 = state2;

    int state3 = 1;
    int previous_state3 = state3;

    int state4 = 1;
    int previous_state4 = state4;

    int config_option = 0;
    int biggest_config_string = 0;

    while (1)
    {
        state1 = gpio_get_level(BUTTON_PIN1);
        state2 = gpio_get_level(BUTTON_PIN2);
        state3 = gpio_get_level(BUTTON_PIN3);
        state4 = gpio_get_level(BUTTON_PIN4);

        if (state1 != previous_state1)
        {
            previous_state1 = state1;

            if (!state1)
            {
                if (is_choose_config_menu_on)
                {
                    hd44780_clear(&lcd);
                    is_choose_config_menu_on = 0;
                    is_config_menu_on = 0;
                }
                else
                {
                    hd44780_clear(&lcd);
                    if (is_config_menu_on)
                    {
                        is_choose_config_menu_on = 0;
                    }
                    else
                    {

                        is_choose_config_menu_on = 1;
                    }
                    is_config_menu_on = 0;
                }
            }
        }

        if (state2 != previous_state2)
        {
            previous_state2 = state2;

            if (!state2)
            {
                if (is_choose_config_menu_on)
                {
                    hd44780_clear(&lcd);
                    is_choose_config_menu_on = 0;
                    is_config_menu_on = 1;
                }
                else
                {
                    if (is_config_menu_on)
                    {
                        hd44780_clear(&lcd);
                        is_choose_config_menu_on = 1;
                        is_config_menu_on = 0;
                    }
                }
            }
        }

        if (state3 != previous_state3)
        {
            previous_state3 = state3;

            if (!state3)
            {
                if (is_choose_config_menu_on)
                {
                    hd44780_clear(&lcd);
                    config_option = !config_option;
                }

                if (is_config_menu_on)
                {
                    if (config_option)
                    {

                        if (temperature_level < 50)
                        {
                            temperature_level++;
                        }
                    }
                    if (!config_option)
                    {

                        if (distance_percentage < MAX_DISTANCE)
                        {
                            distance_percentage += 5;
                        }
                    }
                }
            }
        }

        if (state4 != previous_state4)
        {
            previous_state4 = state4;

            if (!state4)
            {
                if (is_choose_config_menu_on)
                {
                    hd44780_clear(&lcd);
                    config_option = !config_option;
                }

                if (is_config_menu_on)
                {
                    if (config_option)
                    {
                        temperature_level--;
                    }
                    if (!config_option)
                    {
                        if (distance_percentage > MIN_DISTANCE)
                        {
                            distance_percentage -= 5;
                        }
                    }
                }
            }
        }

        if (is_choose_config_menu_on)
        {
            write_on_lcd("Choose config", 0);
            write_on_lcd(config_option ? "2. Temp" : "1. Distance", 1);
        }

        if (is_config_menu_on)
        {
            char config_string[LCD_STRING_SIZE];

            memset(config_string, ' ', LCD_STRING_SIZE);
            config_string[LCD_STRING_SIZE] = '\0';

            snprintf(config_string, sizeof(config_string), config_option ? "Temp = %d" : "Dist = %d%%", config_option ? temperature_level : distance_percentage);

            if (strlen(config_string) < biggest_config_string)
            {
                hd44780_clear(&lcd);
            }

            biggest_config_string = strlen(config_string);

            write_on_lcd("Set config", 0);
            write_on_lcd(config_string, 1);
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}