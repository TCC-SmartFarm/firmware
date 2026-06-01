/*

Arquivo contendo os headers das funções implementadas no arquivo serial_cli.c

*/

// Include guards
#ifndef SERIAL_CLI_H
#define DSERIAL_CLI_H

// Includes 
#include "esp_err.h"

/**
 * @brief Inicializa a interface de linha de comando UART.
 * * Cria uma Task para lidar com o menu de input do usuário
 * * @return ESP_OK em caso de sucesso na instalação do driver e criação da tarefa,
 * ou código de erro correspondente.
 */
esp_err_t cli_config_start(void);

#endif