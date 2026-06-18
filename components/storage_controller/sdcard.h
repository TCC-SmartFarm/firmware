/*

Arquivo contendo os headers das funções implementadas no arquivo sdcard.c

*/

// Include guards
#ifndef SDCARD_H
#define SDCARD_H

// Includes
#include "esp_err.h"
#include <stddef.h>
#include "hal/spi_types.h"


/**
 * @brief Configura o dispositivo SD e monta o sistema de arquivos no VFS.
 * * @param host_id Identificador do barramento SPI previamente inicializado (ex: SPI2_HOST).
 * @param cs_pin Pino de Chip Select (CS) específico para o módulo do cartão SD.
 * @param mount_point Caminho de montagem no Virtual File System (ex: "/sdcard").
 * @return esp_err_t ESP_OK em caso de sucesso, ou código de erro específico.
 */
esp_err_t sdcard_config(spi_host_device_t host_id, int cs_pin, const char* mount_point);

/**
 * @brief Abre o arquivo em modo append ("a"), grava a string fornecida e fecha o arquivo.
 * * @param file_path Caminho absoluto do arquivo no VFS (ex: "/sdcard/dados.txt").
 * @param data String terminada em nulo contendo os dados a serem gravados.
 * @return esp_err_t ESP_OK em caso de sucesso.
 */
esp_err_t sdcard_write(const char* file_path, const char* data);

/**
 * @brief Abre o arquivo, desloca o ponteiro de leitura para o offset especificado, 
 * lê um bloco de dados para o buffer e fecha o arquivo.
 * * @param file_path Caminho absoluto do arquivo no VFS.
 * @param offset Posição em bytes a partir do início do arquivo onde a leitura deve começar.
 * @param buffer Ponteiro para o buffer previamente alocado que receberá os dados.
 * @param buffer_size Tamanho do buffer (quantidade máxima de bytes a ler nesta chamada).
 * @param bytes_read Ponteiro para armazenar a quantidade real de bytes lidos.
 * @return esp_err_t ESP_OK em caso de sucesso.
 */
esp_err_t sdcard_read_chunk(const char* file_path, size_t offset, char* buffer, size_t buffer_size, size_t* bytes_read);

/**
 * @brief Desmonta o sistema de arquivos FatFs e libera a estrutura do dispositivo SD.
 * * @param mount_point Caminho de montagem definido em sdcard_config.
 * @return esp_err_t ESP_OK em caso de sucesso.
 */
esp_err_t sdcard_unmount(const char* mount_point);

/**
 * @brief Função para teste do ciclo de vida do cartão SD.
 * * @param host_id Identificador do barramento SPI previamente inicializado (ex: SPI2_HOST).
 * @param cs_pin Pino de Chip Select (CS) específico para o módulo do cartão SD.
 * @return esp_err_t ESP_OK em caso de sucesso.
 */
esp_err_t sdcard_debug_lifecycle(spi_host_device_t host_id, int cs_pin);

/**
 * @brief Calcula o espaço livre disponível no cartão SD.
 * @return uint64_t Espaço livre em Kilobytes (KB). Retorna 0 em caso de falha.
 */
uint64_t sdcard_get_free_space_kb(void);

#endif 