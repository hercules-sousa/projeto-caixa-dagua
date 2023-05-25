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

//         printf("Distancia: %.2f cm\n", distance);

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

#include <stdio.h>
#include <esp_log.h>
// #include <owb.h>
// #include <owb_rmt.h>
#include <ds18b20.h>

static const char *TAG = "DS18B20 Example";

void app_main(void)
{
    // Inicialização do barramento 1-Wire
    OneWireBus *owb;
    owb_rmt_driver_info rmt_driver_info;
    owb = owb_rmt_initialize(&rmt_driver_info, GPIO_NUM_XX, RMT_CHANNEL_X, RMT_CHANNEL_X);

    // Inicialização do sensor DS18B20
    DS18B20_Info *ds18b20_info = ds18b20_malloc();
    ds18b20_init(ds18b20_info, owb);

    // Local para armazenar a temperatura
    float temperature = 0;

    // Loop principal
    while (1)
    {
        // Leitura da temperatura
        ds18b20_convert(ds18b20_info, NULL);
        ds18b20_wait_for_conversion(ds18b20_info);
        ds18b20_read_temperature(ds18b20_info, &temperature);

        // Exibir a temperatura
        ESP_LOGI(TAG, "Temperatura: %.2f°C", temperature);

        // Delay entre as leituras
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
