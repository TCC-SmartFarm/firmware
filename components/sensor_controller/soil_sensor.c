/*

Arquivo destinado a implementar as funções de leitura do sensor de umidade do solo. Aceita tanto leituras via pino analógico quanto utilizando o conversor ADS1115

*/


// Includes
#include "soil_sensor.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"

// ------------------------------------------------------ Definição de constantes ------------------------------------------------------

static const char *TAG = "soil_sensor";

// Variáveis de Calibração - Definidas arbitrariamente antes dos testes reais
#define VALOR_SECO 3000    
#define VALOR_MOLHADO 1000 


// Modo ADC Interno
static adc_oneshot_unit_handle_t adc_handle = NULL;
static adc_channel_t adc_chan = -1;

// Modo ADS1115
static i2c_port_t ads_i2c_num = -1;
static uint8_t ads_i2c_address = 0;
static int ads_chan_internal = -1;


// ------------------------------------------------------ Funções ------------------------------------------------------

esp_err_t soil_sensor_init_internal_adc(adc_unit_t adc_unit, adc_channel_t channel) {
    if (adc_handle != NULL) {
            ESP_LOGW(TAG, "ADC interno já inicializado.");
            return ESP_OK;
        }

    // Configuração do ADC Interno
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = adc_unit,
    };

    // Inicialização do ADC Interno
    esp_err_t ret = adc_oneshot_new_unit(&init_config, &adc_handle);
    if (ret != ESP_OK) return ret;

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12, // Permite leitura de tensão até ~3.3V
    };
    ret = adc_oneshot_config_channel(adc_handle, channel, &config);
    if (ret != ESP_OK) return ret;

    adc_chan = channel;
    
    ESP_LOGI(TAG, "Higrômetro inicializado via ADC interno.");
    return ESP_OK;
}



esp_err_t soil_sensor_init_ads1115(i2c_port_t i2c_num, uint8_t i2c_address, int ads_chan) {

    if (ads_chan < 0 || ads_chan > 3) {
        ESP_LOGE(TAG, "Canal ADS1115 inválido. Deve ser entre 0 e 3.");
        return ESP_ERR_INVALID_ARG;
    }

    ads_i2c_num = i2c_num;
    ads_i2c_address = i2c_address;
    ads_channel = ads_chan;
    
    ESP_LOGI(TAG, "Higrômetro configurado para usar ADS1115 via I2C.");
    return ESP_OK;
}