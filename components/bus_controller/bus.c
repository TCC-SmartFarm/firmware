/*

Arquivodestinado a realizar a configuração dos barramentos SPI e I2C a serem compartilhados entre diferentes módulos.

*/

// Inlcudes
#include "shared_bus_manager.h"
#include "esp_log.h"

static const char *TAG = "shared_bus";

esp_err_t shared_bus_spi_init(spi_host_device_t host_id, int mosi_pin, int miso_pin, int sclk_pin, int max_transfer_sz) {
    // Configuração física do barramento (Ref: pág. 1481)
    spi_bus_config_t bus_config = {
        .mosi_io_num = mosi_pin,
        .miso_io_num = miso_pin,
        .sclk_io_num = sclk_pin,
        .quadwp_io_num = -1, // Não utilizado em SPI padrão
        .quadhd_io_num = -1, // Não utilizado em SPI padrão
        .max_transfer_sz = max_transfer_sz
    };

    ESP_LOGI(TAG, "Inicializando barramento SPI (Host ID: %d)", host_id);

    // Inicialização com alocação automática de canal DMA (Ref: pág. 1480)
    esp_err_t ret = spi_bus_initialize(host_id, &bus_config, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha na inicialização do barramento SPI (%s)", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Barramento SPI inicializado com sucesso.");
    return ESP_OK;
}