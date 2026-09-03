/*

Arquivo contendo os headers das funções implementadas no arquivo battery_level.c

*/

// Include Guards
#ifndef BATTERY_LEVEL
#define BATTERY_LEVEL


// Includes
#include "esp_err.h"    
#include "driver/i2c.h"

/**
 * @brief Inicializa o divisor resistivo usando o conversor externo ADS1115 via I2C.
 * @param i2c_num Porta I2C previamente inicializada pelo bus_manager.
 * @param i2c_address Endereço I2C físico do ADS1115 (padrão 0x48 - ADDR ligado em GND).
 * @param ads_channel Canal analógico interno do ADS1115 conectado à bateria.
 * @return esp_err_t ESP_OK em caso de sucesso.
 */
esp_err_t battery_level_init(i2c_port_t i2c_num, uint8_t i2c_address, int ads_chan);

/**
 * @brief Lê a tensão da bateria e converte para porcentagem.
 * @param battery_level Ponteiro para armazenar o nível calculado (0% a 100%).
 * @return esp_err_t ESP_OK em caso de sucesso.
 */
esp_err_t battery_level_read(float* battery_level);

#endif