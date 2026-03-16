#ifndef AIR_SENSOR_H
#define AIR_SENSOR_H

#include "esp_err.h"


/**
 * @brief Inicializa o sensor de ar, configurando o pino de comunicação.
 * @param pin Pino GPIO onde o sensor está conectado.
 * @return esp_err_t ESP_OK em caso de sucesso.
 */
esp_err_t air_sensor_init(int pin);

/**
 * @brief Lê os valores de temperatura e umidade do sensor.
 * @param temperature Ponteiro para armazenar a temperatura lida (em graus Celsius).
 * @param humidity Ponteiro para armazenar a umidade lida (em percentual).
 * @return esp_err_t ESP_OK em caso de sucesso, ou erro de leitura/timeout.
 */
esp_err_t air_sensor_read(float* temperature, float* humidity);

#endif