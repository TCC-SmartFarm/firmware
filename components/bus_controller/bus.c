/*

Arquivo destinado a realizar a configuração dos barramentos SPI e I2C a serem compartilhados entre diferentes módulos.

*/

// Inlcudes
#include "bus.h"
#include "esp_log.h"
#include "driver/sdspi_host.h"
#include "hal/spi_types.h"

static const char *TAG = "bus";

esp_err_t bus_spi_init(spi_host_device_t host_id, int mosi_pin, int miso_pin, int sclk_pin, int max_transfer_sz) {
    // Configuração física do barramento
    spi_bus_config_t bus_config = {
        .mosi_io_num = mosi_pin,
        .miso_io_num = miso_pin,
        .sclk_io_num = sclk_pin,
        .quadwp_io_num = -1, // Não utilizado 
        .quadhd_io_num = -1, // Não utilizado
        .max_transfer_sz = max_transfer_sz
    };

    ESP_LOGI(TAG, "Inicializando barramento SPI (Host ID: %d)", host_id);

    // Inicializacao do barramento
    esp_err_t ret = spi_bus_initialize(host_id, &bus_config, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha na inicialização do barramento SPI (%s)", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Barramento SPI inicializado com sucesso.");
    return ESP_OK;
}

esp_err_t bus_spi_free(spi_host_device_t host_id) {
    // Libera os recursos alocados para o barramento
    esp_err_t ret = spi_bus_free(host_id);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao liberar barramento SPI (Host ID: %d) - %s", host_id, esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Barramento SPI liberado com sucesso.");
    return ESP_OK;
}