/*

Arquivo contendo o loop principal de execução.

*/



#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "bus.h" // Importando a sua camada de abstração

// =========================================================
// ATUALIZE ESTES VALORES PARA OS SEUS PINOS REAIS
#define PIN_NUM_MISO 19 
#define PIN_NUM_MOSI 23 
#define PIN_NUM_CLK  18 
#define PIN_NUM_CS   26 
#define PIN_NUM_RST  25 
// =========================================================

static const char *TAG = "SPI_BRIDGE_TEST";

void app_main(void) {
    ESP_LOGI(TAG, "Iniciando teste de integração com o bus_controller...");
    vTaskDelay(pdMS_TO_TICKS(10000));

    // 1. Reset manual do chip (Ainda necessário pois não estamos usando a RadioLib)
    ESP_LOGI(TAG, "Aplicando Reset no pino %d...", PIN_NUM_RST);
    gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_NUM_RST, 0); 
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(PIN_NUM_RST, 1); 
    vTaskDelay(pdMS_TO_TICKS(10));

    // 2. Inicialização usando a SUA função do bus_controller
    ESP_LOGI(TAG, "Chamando bus_spi_init...");
    esp_err_t err = bus_spi_init(SPI2_HOST, PIN_NUM_MOSI, PIN_NUM_MISO, PIN_NUM_CLK, 32);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bus_spi_init falhou: %s", esp_err_to_name(err));
        return;
    }

    // 3. Adicionar o dispositivo ao barramento (Mock do que o EspHal deveria fazer)
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1000000, // 1 MHz
        .mode = 0,                 
        .spics_io_num = PIN_NUM_CS, // CS controlado pelo hardware neste teste
        .queue_size = 1,
    };
    
    spi_device_handle_t spi;
    err = spi_bus_add_device(SPI2_HOST, &devcfg, &spi);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao adicionar dispositivo na via SPI: %s", esp_err_to_name(err));
        return;
    }

    // 4. Transação SPI: Ler o Registrador 0x42 (RegVersion)
    uint8_t tx_data[2] = { 0x42, 0x00 }; 
    uint8_t rx_data[2] = { 0x00, 0x00 }; 

    spi_transaction_t t = {
        .length = 16, 
        .tx_buffer = tx_data,
        .rx_buffer = rx_data
    };

    ESP_LOGI(TAG, "Lendo o registrador 0x42 (RegVersion)...");
    err = spi_device_transmit(spi, &t);

    if (err == ESP_OK) {
        if (rx_data[1] == 0x12 || rx_data[1] == 0x22) {
            ESP_LOGI(TAG, "[SUCESSO] bus_controller operante! Versão lida: 0x%02X", rx_data[1]);
        } else {
            ESP_LOGE(TAG, "[FALHA] bus_controller não comunicou corretamente. Retorno: 0x%02X", rx_data[1]);
        }
    }

    // Limpeza (Testando o bus_spi_free também)
    spi_bus_remove_device(spi);
    bus_spi_free(SPI2_HOST);
}