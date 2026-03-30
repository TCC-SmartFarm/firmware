/*

Arquivo contendo os headers das funções implementadas no arquivo light_sensor.c

*/



// Include Guards
#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H


// Includes
#include "esp_err.h"    
#include "driver/i2c.h"

/**
 * @brief Inicializa o sensor de luz utilizando o conversor externo ADS1115 via I2C.
 * @param i2c_num Porta I2C previamente inicializada pelo bus_manager.
 * @param i2c_address Endereço I2C físico do ADS1115 (padrão 0x48 - ADDR ligado em GND).
 * @param ads_channel Canal analógico interno do ADS1115 conectado ao sensor de luz.
 * @return esp_err_t ESP_OK em caso de sucesso.
 */
esp_err_t light_sensor_init(i2c_port_t i2c_num, uint8_t i2c_address, int ads_chan);

/**
 * @brief Lê a intensidade luminosa e converte para porcentagem.
 * @param light_percent Ponteiro para armazenar a luminosidade calculada (0% a 100%).
 * @return esp_err_t ESP_OK em caso de sucesso.
 */
esp_err_t light_sensor_read(float* light_percent);

#endif