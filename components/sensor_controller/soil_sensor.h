#ifndef SOIL_SENSOR_H
#define SOIL_SENSOR_H


// Includes
#include "esp_err.h"
#include "driver/i2c.h"      
#include "hal/adc_types.h"   

/**
 * @brief Inicializa o higrômetro usando o ADC interno do ESP32.
 * @param adc_unit Unidade do ADC (ex: ADC_UNIT_1).
 * @param channel Canal do ADC correspondente ao pino GPIO do sensor.
 * @return esp_err_t ESP_OK em caso de sucesso.
 */
esp_err_t soil_sensor_init_internal_adc(adc_unit_t adc_unit, adc_channel_t channel);

/**
 * @brief Inicializa o higrômetro usando o conversor externo ADS1115 via barramento I2C.
 * @param i2c_num Porta I2C previamente inicializada.
 * @param i2c_address Endereço I2C físico do ADS1115.
 * @param ads_channel Canal analógico interno do ADS1115  conectado ao sensor.
 * @return esp_err_t ESP_OK em caso de sucesso.
 */
esp_err_t soil_sensor_init_ads1115(i2c_port_t i2c_num, uint8_t i2c_address, int ads_channel);

/**
 * @brief Lê a umidade do solo utilizando o ADC interno.
 * @param moisture_percent Ponteiro para armazenar a umidade calculada (0% a 100%).
 * @return esp_err_t ESP_OK em caso de sucesso.
 */
esp_err_t soil_sensor_read_internal_adc(float* moisture_percent);

/**
 * @brief Lê a umidade do solo utilizando o ADS1115 via I2C.
 * @param moisture_percent Ponteiro para armazenar a umidade calculada (0% a 100%).
 * @return esp_err_t ESP_OK em caso de sucesso.
 */
esp_err_t soil_sensor_read_ads1115(float* moisture_percent);

#endif