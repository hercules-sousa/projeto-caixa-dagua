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

#define TRIGGER_PIN 1
#define ECHO_PIN 0
#define YELLOW_LED_PIN 8
#define MAX_TANK_LEVEL_DISTANCE 30
#define MIN_TANK_LEVEL_DISTANCE 161
#define TANK_SIZE MIN_TANK_LEVEL_DISTANCE - MAX_TANK_LEVEL_DISTANCE

static const gpio_num_t SENSOR_GPIO = 13;
static const int MAX_SENSORS = 1;
static const int RESCAN_INTERVAL = 8;
static const uint32_t LOOP_DELAY_MS = 500;

static const char *TAG = "measure_temperature";

float measureDistance()
{
    // Enviar pulso de disparo
    gpio_set_level(TRIGGER_PIN, 1);
    ets_delay_us(10);
    gpio_set_level(TRIGGER_PIN, 0);

    // Aguardar o pulso de eco
    while (gpio_get_level(ECHO_PIN) == 0)
        ;

    // Iniciar a contagem do tempo
    uint64_t start_time = esp_timer_get_time();

    // Aguardar o fim do pulso de eco
    while (gpio_get_level(ECHO_PIN) == 1)
        ;

    // Calcular o tempo de viagem do pulso de eco
    uint64_t end_time = esp_timer_get_time();
    uint64_t travel_time = end_time - start_time;

    // Calcular a distância em centímetros
    float distance = (float)travel_time / 58.0;

    return distance;
}

void distanceTask(void *pvParameters)
{
    float distance;

    while (1)
    {
        distance = measureDistance();

        if (distance > 39.1)
        {
            gpio_set_level(YELLOW_LED_PIN, 1);
        }
        else
        {
            gpio_set_level(YELLOW_LED_PIN, 0);
        }

        ESP_LOGI(TAG, "Distância = %.2f", distance);

        vTaskDelay(pdMS_TO_TICKS(1000)); // Aguardar 1 segundo
    }
}

void hcsr04_setup()
{
    // Configurar os pinos como GPIO de saída e entrada, respectivamente
    gpio_pad_select_gpio(TRIGGER_PIN);
    gpio_set_direction(TRIGGER_PIN, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(ECHO_PIN);
    gpio_set_direction(ECHO_PIN, GPIO_MODE_INPUT);
}

void measure_temperature(void *pvParameter)
{
    ds18x20_addr_t addrs[MAX_SENSORS];
    float temps[MAX_SENSORS];
    size_t sensor_count = 0;

    gpio_set_pull_mode(SENSOR_GPIO, GPIO_PULLUP_ONLY);

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

                // ESP_LOGI(TAG, "Sensor %08 " PRIx32 "%08" PRIx32 " (%s) reports %.3f°C (%.3f°F)",
                //          (uint32_t)(addrs[j] >> 32), (uint32_t)addrs[j],
                //          (addrs[j] & 0xff) == DS18B20_FAMILY_ID ? "DS18B20" : "DS18S20", temp_c, temp_f);

                ESP_LOGI(TAG, "Temperatura = %.3f°C", temp_c);
            }

            vTaskDelay(pdMS_TO_TICKS(LOOP_DELAY_MS));
        }
    }
}

void led_setup()
{
    gpio_pad_select_gpio(YELLOW_LED_PIN);
    gpio_set_direction(YELLOW_LED_PIN, GPIO_MODE_OUTPUT);
}

void app_main()
{
    hcsr04_setup();
    led_setup();

    gpio_set_level(YELLOW_LED_PIN, 1);

    xTaskCreate(distanceTask, "distanceTask", 2048, NULL, 5, NULL);
    xTaskCreate(measure_temperature, "measure_temperature", configMINIMAL_STACK_SIZE * 4, NULL, 5, NULL);
}
