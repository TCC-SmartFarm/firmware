/*

Arquivo contendo os headers das funções implementadas no arquivo serial_cli.c

*/

// Include guards
#ifndef SERIAL_CLI_H
#define DSERIAL_CLI_H

// Includes 
#include "esp_err.h"

/**
 * @brief Configura o hardware da UART0 e instala o driver correspondente.
 * @return ESP_OK em caso de sucesso.
 */
esp_err_t uart_cli_init(void);

/**
 * @brief Aguarda de forma bloqueante por atividade na porta serial.
 * @param timeout_ms Tempo máximo de espera em milissegundos.
 * @return true se algum caractere for recebido, false se atingir o timeout.
 */
bool uart_cli_wait_for_user(uint32_t timeout_ms);

/**
 * @brief Executa o laço principal e bloqueante do menu interativo.
 * Esta função assume o controle da execução até que o usuário salve e reinicie.
 */
void uart_cli_run_menu(void);

#endif