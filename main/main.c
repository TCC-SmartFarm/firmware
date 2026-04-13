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
#include "soil_sensor.h"
#include "light_sensor.h"

static const char *TAG = "main";

/* ------------------------------ Pinagens e Constantes --------------------------------------*/


// Barramento I²C
#define PIN_NUM_I2C_SCL             22
#define PIN_NUM_I2C_SDA             21
#define I2C_MASTER_NUM              I2C_NUM_0  // Interface I2C Zero do ESP32
#define I2C_FREQ_HZ                 400000    // Alterar este valor para mudar o modo de operação (Fast Mode = 400kHZ)


// Barramento SPI
#define SPI_HOST_ID              SPI2_HOST
#define PIN_NUM_SPI_MISO         19
#define PIN_NUM_SPI_MOSI         23
#define PIN_NUM_SPI_CLK          18
#define SPI_MAX_TRANSFER         4000



// Conversor AD externo - ADS1115
#define ADS1115_I2C_ADDRESS         0x48    // Endereço padrão (ADDR ligado em GND)
#define ADS1115_CHANNEL_HIG         0       // Canal A0 -> Higrômetro
#define ADS1115_CHANNEL_LDR         1       // Canal A1 -> LDR

// Cartão SD
#define PIN_NUM_SPI_CS_SD        5


// DHT
#define PIN_NUM_SDA_DHT             13

// Módulo Lora



/* ------------------------------ Protótipos - Funções de Orquestração --------------------------------------*/

/**
 * @brief Inicializa a infraestrutura de barramentos (SPI e I2C) do sistema.
 * @return esp_err_t ESP_OK se todos os barramentos foram inicializados com sucesso.
 * Retorna o código de erro específico caso algum barramento falhe.
 */

static esp_err_t system_bus_init(void);

void app_main(void) {
    ESP_LOGI(TAG, "\n========== Inicializado! ============\n");



    ESP_LOGI(TAG, "\n========== Fim do teste! ============\n");
}


/* ------------------------------ Funções de Orquestração --------------------------------------*/

static esp_err_t system_bus_init(void) {

    ESP_LOGI(TAG, "\n========== Inicializando Barramentos... ============\n");
    esp_err_t ret;

    // SPI
    ret = shared_bus_spi_init(SPI_HOST_ID, PIN_NUM_SPI_MOSI, PIN_NUM_SPI_MISO, PIN_NUM_SPI_CLK, SPI_MAX_TRANSFER);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha critica: Nao foi possivel inicializar o barramento SPI.");
        return ret;
    }

    // I2C
    ret = shared_bus_i2c_init(I2C_MASTER_NUM, PIN_NUM_I2C_SDA, PIN_NUM_I2C_SCL, I2C_FREQ_HZ);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha critica: Nao foi possivel inicializar o barramento I2C.");
        
        // Liberação do barramento em caso de falha
        shared_bus_spi_free(SPI_HOST_ID);
        
        return ret;
    }

    // Ok!
    ESP_LOGI(TAG, "Todos os barramentos inicializados com sucesso.");
    return ESP_OK;
}
