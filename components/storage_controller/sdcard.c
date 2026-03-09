/*
Arquivo destinado a definir funções de mais alto nível para as funcionalidade do cartão sd
a partir da funções providas pelo driver.
*/

// Includes
#include "sdcard.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "esp_log.h"
#include <stdio.h>

// Definição de constantes
static const char *TAG = "sdcard";
static sdmmc_card_t *card; 

esp_err_t sdcard_config(spi_host_device_t host_id, int cs_pin, const char* mount_point) {
    esp_err_t ret;

    // 1. Configuração de montagem do sistema de arquivo
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false, // Em falso para evitar formatação acidental de dados de sensores
        .max_files = 3,                  // Limite de ficheiros abertos simultaneamente
        .allocation_unit_size = 16 * 1024
    };

    ESP_LOGI(TAG, "Iniciando o cartão SD...");

    // 2. Configuração do host SD via SPI 
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = host_id; // Acopla ao barramento SPI externo já inicializado

    // 3. Configuração do pino CS e host no dispositivo
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = cs_pin;
    slot_config.host_id = host_id;

    // 4. Montagem no VFS 
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Falha na montagem do sistema de arquivos.");
        } else {
            ESP_LOGE(TAG, "Falha na inicialização do cartão SD (%s).", esp_err_to_name(ret));
        }
        return ret;
    }

    ESP_LOGI(TAG, "Sistema de arquivos montado com sucesso em: %s", mount_point);
    
    return ESP_OK;
}

esp_err_t sdcard_write(const char* file_path, const char* data) {
    if (file_path == NULL || data == NULL) {
        ESP_LOGE(TAG, "Caminho do arquivo ou dados inválidos.");
        return ESP_ERR_INVALID_ARG;
    }

    // Abertura do arquivo
    FILE* f = fopen(file_path, "a");
    if (f == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir o arquivo %s para escrita.", file_path);
        return ESP_FAIL;
    }

    // Gravação dos dados com adição da quebra de linha
    int res = fprintf(f, "%s\n", data);
    
    if (res < 0) {
        ESP_LOGE(TAG, "Falha ao escrever os dados no arquivo %s.", file_path);
        fclose(f);
        return ESP_FAIL;
    }

    // Fechando o arquivo
    fclose(f);
    
    ESP_LOGI(TAG, "Dados gravados com sucesso em %s", file_path);
    return ESP_OK;
}


esp_err_t sdcard_read_chunk(const char* file_path, size_t offset, char* buffer, size_t buffer_size, size_t* bytes_read) {
    if (file_path == NULL || buffer == NULL || bytes_read == NULL || buffer_size == 0) {
        ESP_LOGE(TAG, "Argumentos inválidos para leitura.");
        return ESP_ERR_INVALID_ARG;
    }

    // Abre o ficheiro em modo "r" (leitura).
    FILE* f = fopen(file_path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir o ficheiro %s para leitura.", file_path);
        return ESP_FAIL;
    }

    // Desloca o ponteiro de leitura para a posição solicitada
    if (fseek(f, offset, SEEK_SET) != 0) {
        ESP_LOGE(TAG, "Falha ao posicionar o ponteiro no offset %zu.", offset);
        fclose(f);
        return ESP_FAIL;
    }

    // Lê até buffer_size - 1 para reservar espaço para o terminador nulo
    *bytes_read = fread(buffer, 1, buffer_size - 1, f);
    
    // Adiciona terminador nulo para que o buffer possa ser tratado de forma segura como string
    buffer[*bytes_read] = '\0';

    fclose(f);
    return ESP_OK;
}