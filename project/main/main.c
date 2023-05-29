// #include <stdio.h>
// #include <freertos/FreeRTOS.h>
// #include <freertos/task.h>
// #include "esp_system.h"
// #include "driver/gpio.h"
// #include "esp_log.h"
// #include "esp_timer.h"

// #define TRIGGER_PIN 1
// #define ECHO_PIN 0

// float measureDistance()
// {
//     // Enviar pulso de disparo
//     gpio_set_level(TRIGGER_PIN, 1);
//     ets_delay_us(10);
//     gpio_set_level(TRIGGER_PIN, 0);

//     // Aguardar o pulso de eco
//     while (gpio_get_level(ECHO_PIN) == 0)
//         ;

//     // Iniciar a contagem do tempo
//     uint64_t start_time = esp_timer_get_time();

//     // Aguardar o fim do pulso de eco
//     while (gpio_get_level(ECHO_PIN) == 1)
//         ;

//     // Calcular o tempo de viagem do pulso de eco
//     uint64_t end_time = esp_timer_get_time();
//     uint64_t travel_time = end_time - start_time;

//     // Calcular a distância em centímetros
//     float distance = (float)travel_time / 58.0;

//     return distance;
// }

// void distanceTask(void *pvParameters)
// {
//     float distance;

//     while (1)
//     {
//         distance = measureDistance();

//         printf("Distancia: %.2f cmm\n", distance);

//         vTaskDelay(pdMS_TO_TICKS(1000)); // Aguardar 1 segundo
//     }
// }

// void setup()
// {
//     // Configurar os pinos como GPIO de saída e entrada, respectivamente
//     gpio_pad_select_gpio(TRIGGER_PIN);
//     gpio_set_direction(TRIGGER_PIN, GPIO_MODE_OUTPUT);
//     gpio_pad_select_gpio(ECHO_PIN);
//     gpio_set_direction(ECHO_PIN, GPIO_MODE_INPUT);
// }

// void app_main()
// {
//     setup();

//     // Criar a tarefa para medir a distância
//     xTaskCreate(distanceTask, "distanceTask", 2048, NULL, 5, NULL);
// }

// #include <stdio.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "string.h"
// #include "esp_system.h"
// #include "nvs_flash.h"
// #include "ds18b20.h"   //Include library
// const int DS_PIN = 18; // GPIO where you connected ds18b20

// void mainTask(void *pvParameters)
// {
//     ds18b20_init(DS_PIN);

//     while (1)
//     {
//         printf("Temperature: %0.1f\n", ds18b20_get_temp());
//         vTaskDelay(1000 / portTICK_PERIOD_MS);
//     }
// }

// void app_main()
// {
//     nvs_flash_init();
//     xTaskCreatePinnedToCore(&mainTask, "mainTask", 2048, NULL, 5, NULL, 0);
// }

#include <inttypes.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <ds18x20.h>
#include <esp_log.h>
#include <esp_err.h>

static const gpio_num_t SENSOR_GPIO = 13;
static const int MAX_SENSORS = 1;
static const int RESCAN_INTERVAL = 8;
static const uint32_t LOOP_DELAY_MS = 500;

static const char *TAG = "ds18x20_test";

void ds18x20_test(void *pvParameter)
{
    ds18x20_addr_t addrs[MAX_SENSORS];
    float temps[MAX_SENSORS];
    size_t sensor_count = 0;

    // There is no special initialization required before using the ds18x20
    // routines.  However, we make sure that the internal pull-up resistor is
    // enabled on the GPIO pin so that one can connect up a sensor without
    // needing an external pull-up (Note: The internal (~47k) pull-ups of the
    // ESP do appear to work, at least for simple setups (one or two sensors
    // connected with short leads), but do not technically meet the pull-up
    // requirements from the ds18x20 datasheet and may not always be reliable.
    // For a real application, a proper 4.7k external pull-up resistor is
    // recommended instead!)
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
            ESP_LOGI(TAG, "Measuring...");

            res = ds18x20_measure_and_read_multi(SENSOR_GPIO, addrs, sensor_count, temps);
            if (res != ESP_OK)
            {
                ESP_LOGE(TAG, "Sensors read error %d (%s)", res, esp_err_to_name(res));
                continue;
            }

            for (int j = 0; j < sensor_count; j++)
            {
                float temp_c = temps[j];
                float temp_f = (temp_c * 1.8) + 32;
                // Float is used in printf(). You need non-default configuration in
                // sdkconfig for ESP8266, which is enabled by default for this
                // example. See sdkconfig.defaults.esp8266
                ESP_LOGI(TAG, "Sensor %08" PRIx32 "%08" PRIx32 " (%s) reports %.3f°C (%.3f°F)",
                         (uint32_t)(addrs[j] >> 32), (uint32_t)addrs[j],
                         (addrs[j] & 0xff) == DS18B20_FAMILY_ID ? "DS18B20" : "DS18S20", temp_c, temp_f);
            }

            // Wait for a little bit between each sample (note that the
            // ds18x20_measure_and_read_multi operation already takes at
            // least 750ms to run, so this is on top of that delay).
            vTaskDelay(pdMS_TO_TICKS(LOOP_DELAY_MS));
        }
    }
}

void app_main() { xTaskCreate(ds18x20_test, "ds18x20_test", configMINIMAL_STACK_SIZE * 4, NULL, 5, NULL); }
