/*

Arquivo contendo o loop principal de execução.

*/

// Includes
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "bus.h"
#include "sdcard.h"

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

    // Inicializa o barramento SPI no host SPI2_HOST
    esp_err_t ret = bus_spi_init(SPI2_HOST, PIN_NUM_MOSI_SD, PIN_NUM_MISO_SD, PIN_NUM_CLK_SD, 4000);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha fatal ao inicializar barramento SPI.");
        return; // Interrompe a execução se não houver barramento
    }

    // Executa o ciclo de teste do cartão SD
    ret = sdcard_debug_lifecycle(SPI2_HOST, PIN_NUM_CS_SD);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Teste do SD concluído com sucesso. Arquivo criado no ponto de montagem.");
    } else {
        ESP_LOGE(TAG, "Erro durante o teste do SD.");
    }

    // Liberta o barramento
    bus_spi_free(SPI2_HOST);

    ESP_LOGI(TAG, "Fim do teste de hardware.");
}