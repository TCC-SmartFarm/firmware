/*

Arquivo destinado a realizar a configuração dos barramentos SPI e I2C a serem compartilhados entre diferentes módulos.

*/

// Inlcudes
#include "bus.h"
#include "esp_log.h"
#include "driver/sdspi_host.h"
#include "hal/spi_types.h"

static const char *TAG = "bus";


// ----------------------------------------------------------------- SPI -----------------------------------------------------------------

esp_err_t bus_spi_init(spi_host_device_t host_id, int mosi_pin, int miso_pin, int sclk_pin, int max_transfer_sz) {
    ESP_LOGI(TAG, "Iniciando SPI dinâmico (Host: %d) -> MOSI: %d | MISO: %d | SCK: %d", host_id, mosi_pin, miso_pin, sclk_pin);

    // Inicialização 
    spi_bus_config_t bus_config = {0};

    // Atribuição de pinos 
    bus_config.mosi_io_num = mosi_pin;
    bus_config.miso_io_num = miso_pin;
    bus_config.sclk_io_num = sclk_pin;
    bus_config.quadwp_io_num = -1;
    bus_config.quadhd_io_num = -1;
    bus_config.data4_io_num = -1;
    bus_config.data5_io_num = -1;
    bus_config.data6_io_num = -1;
    bus_config.data7_io_num = -1;
    bus_config.max_transfer_sz = max_transfer_sz;

    // Inicialização com DMA automático 
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

// ----------------------------------------------------------------- I²C -----------------------------------------------------------------

esp_err_t bus_i2c_init(i2c_port_t i2c_num, int sda_pin, int scl_pin, uint32_t clk_speed) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = clk_speed,
    }; // Configuração inicial do barramento I2C

    ESP_LOGI(TAG, "\n ========== Inicializando I²C em: %d ========== \n", i2c_num);

    // Parâmetros do periférico
    esp_err_t ret = i2c_param_config(i2c_num, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "\n ========== Erro na inicialização! ========== \n Trace: (%s) \n", esp_err_to_name(ret));
        return ret;
    }

    // Instalação do driver como Master (3 0's no final)
    ret = i2c_driver_install(i2c_num, conf.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "\n ========== Erro na instalação do driver! ========== \n Trace: (%s) \n", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "\n ========== I²C OK! ========== \n");
    return ESP_OK;
}

esp_err_t bus_i2c_free(i2c_port_t i2c_num) {
    esp_err_t ret = i2c_driver_delete(i2c_num);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "\n ========== Erro na liberação do driver em %d! ========== \n Trace: (%s) \n)", i2c_num, esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "\n ========== I²C Liberado! ========== \n");
    return ESP_OK;
}