/*

Arquivo contendo os headers das funções implementadas no arquivo soil_sensor.c

*/



// Include Guards
#ifndef SOIL_SENSOR_H
#define SOIL_SENSOR_H


// Includes
#include "esp_err.h"    
#include "hal/adc_types.h"   
#include "driver/i2c.h"


/**
 * @brief Inicializa o higrômetro usando o conversor externo ADS1115 via barramento I2C.
 * @param i2c_num Porta I2C previamente inicializada.
 * @param i2c_address Endereço I2C físico do ADS1115 (padrão 0x48 - ADDR ligado em GND).
 * @param ads_channel Canal analógico interno do ADS1115  conectado ao sensor.
 * @return esp_err_t ESP_OK em caso de sucesso.
 */
esp_err_t soil_sensor_init_ads1115(i2c_port_t i2c_num, uint8_t i2c_address, int ads_channel);

/**
 * @brief Lê a umidade do solo utilizando o ADS1115 via I2C.
 * @param moisture_percent Ponteiro para armazenar a umidade calculada (0% a 100%).
 * @return esp_err_t ESP_OK em caso de sucesso.
 */
esp_err_t soil_sensor_read_ads1115(float* moisture_percent);

#endif