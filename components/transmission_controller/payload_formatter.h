/*

Arquivo dedicado a abrigar funções para converter os dados de leitura para uma payload que pode ser transmitida por LoRa e MQTT (rede celular)

*/


#ifndef PAYLOAD_FORMATTER_H
#define PAYLOAD_FORMATTER_H

#include <stdint.h>
#include "sensor_data.h"

// Define o tamanho fixo do payload comprimido para alocacao na main - Ajustar conforme necessário
#define LORAWAN_PAYLOAD_SIZE 12 

// Forward declaration temporaria caso use struct nomeada (struct sensor_data_t)
struct sensor_data_t; 

/**
 * @brief Serializa os dados dos sensores num array binario (Big-Endian).
 * @param data Ponteiro para a estrutura de dados lidos.
 * @param payload_buffer Buffer de saída de 12 bytes.
 */
void lora_payload_formatter(const struct sensor_data_t *data, uint8_t *payload_buffer);

#endif // PAYLOAD_FORMATTER_H