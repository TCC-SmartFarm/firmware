#ifndef LORA_WAN_NODE_H
#define LORA_WAN_NODE_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Struct de configuração de hardware do rádio LoRa.
 */
typedef struct {
    int spi_host_id;    // Identificador do barramento SPI (ex: SPI2_HOST)
    int nss_pin;        // Chip Select (CS) - GPIO 26
    int rst_pin;        // Reset - GPIO 25
    int dio0_pin;       // Interrupção TX/RX - GPIO 32
    int dio1_pin;       // Interrupção LoRaWAN - GPIO 27
} lorawan_hal_config_t;

/**
 * @brief Credenciais criptográficas padrão LoRaWAN 1.0.x (OTAA).
 * Estes valores devem ser extraídos da memória NVS ou definidos no portal.
 */
typedef struct {
    uint8_t dev_eui[8];   // Identificador único do dispositivo
    uint8_t join_eui[8];  // Identificador da aplicação (AppEUI)
    uint8_t app_key[16];  // Chave criptográfica de sessão
} lorawan_keys_t;

/**
 * @brief Inicializa o rádio SX1276 e processa o Join (OTAA) na rede LoRaWAN.
 * @param hal_conf Ponteiro para os pinos e barramento SPI.
 * @param keys Ponteiro para as credenciais da rede.
 * @return ESP_OK se o módulo rádio responder e o Join for bem sucedido.
 */
esp_err_t lorawan_node_init(const lorawan_hal_config_t *hal_conf, const lorawan_keys_t *keys);

/**
 * @brief Transmite um pacote de dados para o Network Server (Uplink).
 * @param f_port Porta lógica do payload (1 a 223).
 * @param data Ponteiro para o buffer de dados (formato empacotado, não string).
 * @param length Tamanho do payload em bytes.
 * @return ESP_OK se o pacote foi transmitido e (se confirmado) o ACK recebido.
 */
esp_err_t lorawan_node_send(uint8_t f_port, const uint8_t *data, size_t length);

/**
 * @brief Coloca o rádio SX1276 em modo Sleep para poupar bateria.
 * Deve ser chamado antes do ESP32 entrar em Deep Sleep.
 */
esp_err_t lorawan_node_sleep(void);

#ifdef __cplusplus
}
#endif

#endif // LORA_WAN_NODE_H