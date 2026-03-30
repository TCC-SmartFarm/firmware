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


// Barramento SPI (?)



// Conversor AD externo - ADS1115
#define ADS1115_I2C_ADDRESS         0x48    // Endereço padrão (ADDR ligado em GND)
#define ADS1115_CHANNEL_HIG         0       // Canal A0 -> Higrômetro
#define ADS1115_CHANNEL_LDR         1       // Canal A1 -> LDR

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

    // 1. Inicializa a infraestrutura do barramento I2C
    esp_err_t ret = bus_i2c_init(I2C_MASTER_NUM, PIN_NUM_I2C_SDA, PIN_NUM_I2C_SCL, I2C_FREQ_HZ);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha na inicialização do barramento I2C.");
        return;
    }

    // 2. Acopla os sensores ao barramento e aos seus respectivos canais
    ret = soil_sensor_init_ads1115(I2C_MASTER_NUM, ADS1115_I2C_ADDRESS, ADS1115_CHANNEL_HIG);
    if (ret != ESP_OK) ESP_LOGE(TAG, "Falha ao inicializar Higrômetro.");

    ret = light_sensor_init(I2C_MASTER_NUM, ADS1115_I2C_ADDRESS, ADS1115_CHANNEL_LDR);
    if (ret != ESP_OK) ESP_LOGE(TAG, "Falha ao inicializar Sensor de Luz.");

    // 3. Loop de amostragem
    while (1) {
        float soil_moisture = 0.0;
        float light_level = 0.0;
        
        // Efetua as leituras sequencialmente
        esp_err_t ret_soil = soil_sensor_read_ads1115(&soil_moisture);
        esp_err_t ret_light = light_sensor_read(&light_level);

        // Formata a saída no terminal para facilitar a visualização
        if (ret_soil == ESP_OK && ret_light == ESP_OK) {
            ESP_LOGI(TAG, "Leituras -> Solo: %5.1f%% | Luz: %5.1f%%", soil_moisture, light_level);
        } else {
            ESP_LOGW(TAG, "Falha em uma ou mais leituras. Solo: %s, Luz: %s", 
                     esp_err_to_name(ret_soil), esp_err_to_name(ret_light));
        }

        // Aguarda 2 segundos antes da próxima amostragem conjunta
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    ESP_LOGI(TAG, "\n========== Fim do teste! ============\n");
}