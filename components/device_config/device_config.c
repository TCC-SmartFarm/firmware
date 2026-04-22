/*

Arquivo destinado a realizar a configuração do Non-Volatile-Storage do ESP a fim de guardar informações de configuração do usuário.

*/

// Includes
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
        ESP_LOGI(TAG, "Subsistema NVS inicializado com sucesso.");
    }
    
    return ret;
}