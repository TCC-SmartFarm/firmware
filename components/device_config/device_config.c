/*

Arquivo destinado a realizar a configuração do Non-Volatile-Storage do ESP a fim de guardar informações de configuração do usuário.

*/

// Includes
#include <sys/time.h>
#include <time.h>

#include "device_config.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

static const char *TAG = "device_config";
static const char *NVS_NAMESPACE = "user_cfg";
static const char *NVS_BLOB_KEY = "config_blob";

esp_err_t device_config_init(void) {
    // Inicialização
    esp_err_t ret = nvs_flash_init();
    
    // Verificação de integridade
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erro no NVS. Resetando e tentando novamente");
        ESP_ERROR_CHECK(nvs_flash_erase()); // Reset
        ret = nvs_flash_init(); // Reinicialização
    }
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "NVS inicializado com sucesso.");
    }
    
    return ret;
}


esp_err_t device_config_save(const user_config_t *config) {
    
    nvs_handle_t nvs_handle;
    esp_err_t err;

    // Abertura do NVS
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) return err;

    // Gravação do BLOB
    err = nvs_set_blob(nvs_handle, NVS_BLOB_KEY, config, sizeof(user_config_t));
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        return err;
    }

    // Commit no NVS
    err = nvs_commit(nvs_handle);
    
    // Fechamento do sistema de arquivo
    nvs_close(nvs_handle);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Configurações salvas no NVS.");

        // Ajuste do relógio interno
        struct timeval tv = {
            .tv_sec = config->setup_date, 
            .tv_usec = 0
        };
        
        settimeofday(&tv, NULL);
        ESP_LOGI(TAG, "Relógio do sistema sincronizado (Timestamp: %lld).", (long long)config->setup_date);
    }

    return err;
}


esp_err_t device_config_load(user_config_t *out_config) {

    nvs_handle_t nvs_handle;
    esp_err_t err;
    size_t required_size = sizeof(user_config_t);

    // Abre o sistema de arquivos
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Falha ao abrir NVS para leitura. Primeira vez por aqui?");
        return err;
    }

    // Verificação da gravação
    err = nvs_get_blob(nvs_handle, NVS_BLOB_KEY, out_config, &required_size);
    
    // Fechamento do sistema de arquivo
    nvs_close(nvs_handle);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Configurações carregadas da memória Flash.");
    } else {
        ESP_LOGW(TAG, "Nenhum dado de configuração encontrado ou erro na leitura.");
    }

    return err;
}

esp_err_t device_config_reset(void) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    // Abre o sistema de arquivos
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) return err;

    // Apaga o namespace user_cfg
    err = nvs_erase_all(nvs_handle);
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
    }

    // Fechamento do sistema de arquivo
    nvs_close(nvs_handle);
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Configurações de utilizador apagadas com sucesso.");
    }

    return err;
}