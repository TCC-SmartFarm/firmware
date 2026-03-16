/*

Arquivo contendo o loop principal de execução.

*/

// Includes de sistema
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
// Includes dos componetes
#include "bus.h"
#include "sdcard.h"
#include "air_sensor.h"

static const char *TAG = "main";

/* ------------------------------ Pinagens --------------------------------------*/

// Cartão SD
#define PIN_NUM_MISO_SD 19
#define PIN_NUM_MOSI_SD 23
#define PIN_NUM_CLK_SD  18
#define PIN_NUM_CS_SD   5

// LDR


// DHT
#define PIN_NUM_SDA_DHT 32

// Higrometro

// Módulo Lora

void app_main(void) {
    ESP_LOGI(TAG, "\n========== Inicializado! ============\n");

    esp_err_t ret = air_sensor_init(PIN_NUM_SDA_DHT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha na inicialização do sensor.");
        return; // Interrompe se o pino for inválido
    }

        while (1) {

            vTaskDelay(pdMS_TO_TICKS(2000));

            float temp = 0.0;
            float hum = 0.0;
            
            ret = air_sensor_read(&temp, &hum);

            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Leitura OK - Temp: %.1fC, Hum: %.1f%%", temp, hum);
            } else {
                ESP_LOGE(TAG, "Falha na leitura.");
            }
        }

    ESP_LOGI(TAG, "\n========== Fim do teste! ============\n");
}