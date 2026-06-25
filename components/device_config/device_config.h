/*

Arquivo contendo os headers das funções implementadas no arquivo device_config.c

*/

// Include guards
#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

// Includes 
#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief Struct de configuração.
 * packed -> Evita padding durante a gravação na memória. 
 */
typedef struct __attribute__((packed)) {
    char device_name[32];
    char password[64];

    uint32_t setup_date; 
    bool is_configured;  // Flag de controle

    // Parametros LoRa - OTAA
    char dev_eui[17];  // 16 caracteres hex + terminador nulo
    char join_eui[17]; // 16 caracteres hex + terminador nulo
    char app_key[33];  // 32 caracteres hex + terminador nulo

    // Persistência de conexão
    bool has_lora_session;
    uint8_t lora_session[256];
    
} user_config_t; // Estrutura atômica

/**
 * @brief Inicialização do NVS.
 */
esp_err_t device_config_init(void);

/**
 * @brief Salva a estrutura completa no NVS.
 * @param config Ponteiro para a struct preenchida.
 */
esp_err_t device_config_save(const user_config_t *config);

/**
 * @brief Carrega a estrutura do NVS para a RAM.
 * @param out_config Ponteiro onde os dados serão copiados.
 */
esp_err_t device_config_load(user_config_t *out_config);

/**
 * @brief Apaga todas as configurações e reseta a flag de setup.
 */
esp_err_t device_config_reset(void);

#endif