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

static const char *TAG = "main";

/* ------------------------------ Pinagens e Constantes --------------------------------------*/


// Barramento I²C
#define PIN_NUM_I2C_SCL             22
#define PIN_NUM_I2C_SDA             21
#define I2C_MASTER_NUM              I2C_NUM_0  // Interface I2C Zero do ESP32
#define I2C_FREQ_HZ             400000    // Alterar este valor para mudar o modo de operação (Fast Mode = 400kHZ)


// Barramento SPI (?)



// Conversor AD externo - ADS1115
#define ADS1115_I2C_ADDRESS         0x48    // Endereço padrão (ADDR ligado em GND)
#define ADS1115_CHANNEL             0       // Canal A0 -> Higrômetro
                                            // Canal A1 -> LDR

// Cartão SD
#define PIN_NUM_SPI_MISO_SD         19
#define PIN_NUM_SPI_MOSI_SD         23
#define PIN_NUM_SPI_CLK_SD          18
#define PIN_NUM_SPI_CS_SD           5


// DHT
#define PIN_NUM_SDA_DHT             32

// Módulo Lora



void app_main(void) {
    ESP_LOGI(TAG, "\n========== Inicializado! ============\n");

    ESP_LOGI(TAG, "Iniciando teste atômico do higrômetro (ADS1115 via I2C)...");

    // 1. Inicializa o barramento I2C
    esp_err_t ret = bus_i2c_init(I2C_MASTER_NUM, PIN_NUM_I2C_SDA, PIN_NUM_I2C_SCL, I2C_FREQ_HZ);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha na inicialização do barramento I2C.");
        return;
    }

    // 2. Inicializa o sensor apontando para o barramento I2C instanciado
    ret = soil_sensor_init_ads1115(I2C_MASTER_NUM, ADS1115_I2C_ADDRESS, ADS1115_CHANNEL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha na inicialização do higrômetro (ADS1115).");
        bus_i2c_free(I2C_MASTER_NUM);
        return;
    }

    // 3. Loop de leitura
    while (1) {
        float moisture = 0.0;
        
        // Efetua a leitura direcionada ao driver I2C
        ret = soil_sensor_read_ads1115(&moisture);

        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Umidade do solo (ADS1115): %.1f%%", moisture);
        } else {
            ESP_LOGE(TAG, "Falha na leitura do sensor via ADS1115.");
        }

        // Aguarda 1 segundo antes da próxima leitura
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGI(TAG, "\n========== Fim do teste! ============\n");
}