#ifndef LORAWAN_CONFIG_H
#define LORAWAN_CONFIG_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

// Buffer de sessão LoraWAN
#define LORAWAN_SESSION_BUF_SIZE 256

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
 * @brief Credenciais estáticas para ativação ABP -> Recomendado para prototipagem (Verificador de frame counter desabilitado no GW)
 */
typedef struct {
    uint32_t dev_addr;      // 4 bytes numéricos
    uint8_t nwk_s_key[16];  // 16 bytes puros 
    uint8_t app_s_key[16];  // 16 bytes puros 
} lorawan_keys_t;

/**
 * @brief Inicializa a comunicação SPI com o transceptor SX1276.
 */
esp_err_t lorawan_hardware_init(const lorawan_hal_config_t *hal_conf);

/**
 * @brief Libera a memória alocada pelo wrapper em C++.
 */
void lorawan_hardware_deinit(void);

/**
 * @brief Ativa o dispositivo na rede LoRaWAN via ABP.
 */
esp_err_t lorawan_activate_abp(const lorawan_keys_t *keys);

/**
 * @brief Salva o estado atual da sessão MAC LoRaWAN.
 */
esp_err_t lorawan_save_session(uint8_t *session_buffer);

/**
 * @brief Restaura o estado da sessão MAC LoRaWAN a partir de um buffer.
 */
esp_err_t lorawan_restore_session(const uint8_t *session_buffer);

/**
 * @brief Transmite um pacote de dados para o Network Server (Uplink).
 */
esp_err_t lorawan_node_send(uint8_t f_port, const uint8_t *data, size_t length);

/**
 * @brief Coloca o rádio SX1276 em modo Sleep.
 */
esp_err_t lorawan_node_sleep(void);

#ifdef __cplusplus
}
#endif

#endif // LORAWAN_CONFIG_H