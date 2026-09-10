/*

Arquivo de headers destinado a instanciação do portal Wi-Fi para configuração.

*/

#ifndef WIFI_PORTAL_H
#define WIFI_PORTAL_H

#include "esp_err.h"


/**
 * @brief Inicializa a interface LwIP, configura o rádio Wi-Fi em modo Access Point (SoftAP)
 * e sobe o servidor HTTP para escutar requisições na porta 80.
 * 
 * @return ESP_OK se o portal foi levantado com sucesso, ou o erro correspondente do ESP-IDF.
 */
esp_err_t wifi_portal_start(void);

/**
 * @brief Desliga o servidor HTTP, destrói a interface de rede e desenergiza o rádio Wi-Fi.
 * 
 */
void wifi_portal_stop(void);


/**
 * @brief Converte a string de data recebida do portal para Epoch
 * 
 */
static uint32_t parse_datetime_to_epoch(const char *datetime_str);


#endif // WIFI_PORTAL_H