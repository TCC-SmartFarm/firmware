/*
Definição de funções de mais alto nível para as funcionalidade do cartão sd
a partir da funções providas pelo driver.
*/

// Includes
#include "sdcard.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "ff.h"
#include "driver/sdspi_host.h"
#include "esp_log.h"
#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>

// Definição de constantes
static const char *TAG = "sdcard";
static sdmmc_card_t *sd_card_handle; 

esp_err_t sdcard_config(spi_host_device_t host_id, int cs_pin, const char* mount_point) {
    esp_err_t ret;

    //Configuração de montagem do sistema de arquivo
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false, 
        .max_files = 3,                  
        .allocation_unit_size = 16 * 1024
    };

    ESP_LOGI(TAG, "Iniciando o cartão SD...");

    ESP_LOGI(TAG, "Tentando acoplar ao barramento...");
    // Configuração do host SD via SPI 
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = host_id; // Acopla ao barramento SPI externo já inicializado
    host.max_freq_khz = 400; // Reduçõa do clock para testes no protoboard

    ESP_LOGI(TAG, "Tentando configurar o pino de CS...");
    //Configuração do pino CS e host no dispositivo
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = cs_pin;
    slot_config.host_id = host_id;

    // Montagem no sistema de arquivo virtual do ESP 
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &sd_card_handle);

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
        ESP_LOGE(TAG, "Falha ao abrir o arquivo %s para escrita., Erro: %s (%d)", file_path, strerror(errno), errno);
        return ESP_FAIL;
    }

    // Gravação dos dados com adição da quebra de linha
    int res = fprintf(f, "%s\n", data);
    
    if (res < 0) {
        ESP_LOGE(TAG, "Falha ao escrever os dados no arquivo %s., Erro: %s (%d)", file_path, strerror(errno), errno);
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

    FILE* f = fopen(file_path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir o ficheiro %s para leitura.", file_path);
        return ESP_FAIL;
    }

    if (fseek(f, offset, SEEK_SET) != 0) {
        ESP_LOGE(TAG, "Falha ao posicionar o ponteiro no offset %zu.", offset);
        fclose(f);
        return ESP_FAIL;
    }

    *bytes_read = fread(buffer, 1, buffer_size - 1, f);

    buffer[*bytes_read] = '\0';

    fclose(f);
    return ESP_OK;
}

/*
esp_err_t sdcard_unmount(const char* mount_point) {
    if (mount_point == NULL) {
        ESP_LOGE(TAG, "Ponto de montagem inválido.");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = esp_vfs_fat_sdcard_unmount(mount_point, card);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao desmontar o cartão SD no ponto %s (%s).", mount_point, esp_err_to_name(ret));
        return ret;
    }


    card = NULL;

    ESP_LOGI(TAG, "Cartão SD desmontado com sucesso do ponto: %s", mount_point);
    return ESP_OK;
}
*/

void sdcard_deinit(const char* mount_point){

    ESP_LOGI(TAG, "=== Iniciando a liberação de recursos do Cartão SD ===");

    if (sd_card_handle != NULL) {

        int spi_device_handle = sd_card_handle->host.slot;

        // Desmontagem do sistema de arquivos
        esp_err_t err = esp_vfs_fat_sdcard_unmount(mount_point, sd_card_handle);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Falha ao desmontar FAT: %s", esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "Sistema FAT desmontado com sucesso.");
        }

        // Liberação do barramento
        err = sdspi_host_remove_device(spi_device_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Falha ao desvincular SD do SPI: %s", esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "Dispositivo SD desvinculado do barramento.");
        }

        //Limpeza do ponteiro
        sd_card_handle = NULL;
        
    }

    ESP_LOGI(TAG, "Cartão SD desmontado com sucesso do ponto: %s", mount_point);
}

uint64_t sdcard_get_free_space_kb(void) {

    FATFS *fs;
    DWORD fre_clust, fre_sect;

    
    FRESULT res = f_getfree("0:", &fre_clust, &fs);
    if (res != FR_OK) {
        ESP_LOGE("sdcard", "Falha ao obter espaco livre do SD (Erro FATFS: %d)", res);
        return 0;
    }

    // Determinando o total de setores livres
    fre_sect = fre_clust * fs->csize;
    
    // Convertendo para Kb
    uint64_t free_space_kb = ((uint64_t)fre_sect * 512) / 1024;
    
    return free_space_kb;
}


