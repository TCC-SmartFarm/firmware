/*

Arquivo destinado a implementar as funções de leitura do sensor de umidade do solo utilizando o conversor ADS1115

*/


// Includes
#include "soil_sensor.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ------------------------------------------------------ Definição de constantes ------------------------------------------------------

static const char *TAG = "soil_sensor";

// Variáveis de Calibração - Definidas arbitrariamente antes dos testes reais
#define VALOR_SECO 3000    
#define VALOR_MOLHADO 1000 


// Inicialização dos parâmetros do ADS1115 (serão modificados pela função de inicialização do sensor)
static i2c_port_t ads_i2c_num = -1;
static uint8_t ads_i2c_address = 0;
static int ads_channel = -1;


// ------------------------------------------------------ Funções ------------------------------------------------------

esp_err_t soil_sensor_init_ads1115(i2c_port_t i2c_num, uint8_t i2c_address, int ads_chan) {

    if (ads_chan < 0 || ads_chan > 3) {
        ESP_LOGE(TAG, "Canal ADS1115 inválido. Deve ser entre 0 e 3.");
        return ESP_ERR_INVALID_ARG;
    }

    ads_i2c_num = i2c_num;
    ads_i2c_address = i2c_address;
    ads_channel = ads_chan;
    
    ESP_LOGI(TAG, "Higrômetro I2C (ADS1115 - Ch %d) OK!.", ads_channel);
    return ESP_OK;
}

// Função de calibração (Tensão lida --> Percentual)
static void calculate_moisture_percentage(int raw_value, float* percent_out) {

    // Conversão com base nos valores máximos
    float percent = (float)(VALOR_SECO - raw_value) / (VALOR_SECO - VALOR_MOLHADO) * 100.0f;

    // Definição dos extremos
    if (percent < 0.0f) percent = 0.0f;
    if (percent > 100.0f) percent = 100.0f;

    *percent_out = percent;
}

esp_err_t soil_sensor_read_ads1115(float* moisture_percent) {

    // Verificação de erro de leitura
    if (moisture_percent == NULL) return ESP_ERR_INVALID_ARG;

    // Verificação da configuração do barramento I²C
    if (ads_i2c_num == -1) {
        ESP_LOGE(TAG, "Higrometro - ADS1115 não inicializado.");
        return ESP_ERR_INVALID_STATE;
    }

    // Palavra de configuração - Config Register do ADS1115
    uint16_t config = 0x4000 | (ads_channel << 12) | // MUX = Single-Ended + Canal
                                            0x0200 | // PGA = Ganho +/- 4.096V
                                            0x0100 | // MODE = Single-Shot
                                            0x0083;  // Data rate = 128 SPS + ALRT off

    config |= 0x8000; // OS = 1
    
    // Valor a ser escrito no Config Register (em 0x01) em dois conjuntos de 8 bits e máscaras
    uint8_t write_buf[3] = {0x01, (uint8_t)(config >> 8), (uint8_t)(config & 0xFF)};

    // Escrita no Config Register via I²C
    esp_err_t ret = i2c_master_write_to_device(ads_i2c_num, ads_i2c_address, write_buf, sizeof(write_buf), pdMS_TO_TICKS(100));
    if (ret != ESP_OK) return ret;

    // Delay para conversão via ADS1115
    vTaskDelay(pdMS_TO_TICKS(10)); 

    // Leitura do registrador
    uint8_t reg_ptr = 0x00; 
    uint8_t read_buf[2] = {0};
    ret = i2c_master_write_read_device(ads_i2c_num, ads_i2c_address, &reg_ptr, 1, read_buf, 2, pdMS_TO_TICKS(100));
    if (ret != ESP_OK) return ret;

    // Composição do valor de 16 bits a partir dos 2 bytes trnasmitidos
    int raw_value = (read_buf[0] << 8) | read_buf[1];
    if (raw_value > 32767) raw_value -= 65536; 
    
    // Conversão 
    calculate_moisture_percentage(raw_value, moisture_percent);
    return ESP_OK;
}