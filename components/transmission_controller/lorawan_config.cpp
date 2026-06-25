#include "lorawan_config.h"
#include "esp_log.h"
#include <RadioLib.h>

static const char *TAG = "lorawan_node";

// Ponteiros para guardar o estado dos componentes C++
static EspHal* hal = nullptr;
static Module* mod = nullptr;
static SX1276* radio = nullptr;
static LoRaWANNode* node = nullptr;

extern "C" esp_err_t lorawan_hardware_init(const lorawan_hal_config_t *hal_conf) {
    if (!hal_conf) {
        ESP_LOGE(TAG, "Configuracoes de hardware invalidas.");
        return ESP_ERR_INVALID_ARG;
    }

    if (!hal) {
        // Acopla ao barramento SPI ja inicializado
        hal = new EspHal((spi_host_device_t)hal_conf->spi_host_id);
        mod = new Module(hal, hal_conf->nss_pin, hal_conf->dio0_pin, hal_conf->rst_pin, hal_conf->dio1_pin);
        radio = new SX1276(mod);
        
        // Frequencia de operação (915MHz)
        node = new LoRaWANNode(radio, &AU915); 
    }

    ESP_LOGI(TAG, "Inicializando transceptor SX1276 via SPI...");
    int16_t state = radio->begin();
    if (state != RADIOLIB_ERR_NONE) {
        ESP_LOGE(TAG, "Falha na inicializacao do SX1276. Erro RadioLib: %d", state);
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

extern "C" esp_err_t lorawan_join(const lorawan_keys_t *keys) {
    if (!node || !keys) {
        ESP_LOGE(TAG, "Motor LoRaWAN nao inicializado ou chaves nulas.");
        return ESP_ERR_INVALID_STATE;
    }

    // Conversão do formato byte array (MSB) para o uint64_t interno da RadioLib
    uint64_t joinEUI = 0;
    uint64_t devEUI = 0;
    for (int i = 0; i < 8; i++) {
        joinEUI = (joinEUI << 8) | keys->join_eui[i];
        devEUI = (devEUI << 8) | keys->dev_eui[i];
    }

    ESP_LOGI(TAG, "Iniciando processo de Join (OTAA)...");
    
    // beginOTAA lida com o Request e aguarda o Accept na janela RX correta
    int16_t state = node->beginOTAA(joinEUI, devEUI, (uint8_t*)keys->app_key, (uint8_t*)keys->app_key);
    
    if (state == RADIOLIB_ERR_NONE) {
        ESP_LOGI(TAG, "Join OTAA aceite pela rede!");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Falha no Join OTAA. Erro RadioLib: %d", state);
        return ESP_FAIL;
    }
}

extern "C" esp_err_t lorawan_save_session(uint8_t *session_buffer) {
    if (!node || !session_buffer) return ESP_ERR_INVALID_STATE;

    // Salva as chaves de sessão para evitar um novo processo de Join
    size_t len = node->saveSession(session_buffer, LORAWAN_SESSION_BUF_SIZE);
    
    if (len > 0) {
        ESP_LOGI(TAG, "Sessao MAC extraida com sucesso (%zu bytes).", len);
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Falha na extracao do estado da sessao.");
        return ESP_FAIL;
    }
}

extern "C" esp_err_t lorawan_restore_session(const uint8_t *session_buffer) {
    if (!node || !session_buffer) return ESP_ERR_INVALID_STATE;

    ESP_LOGI(TAG, "Restaurando estado MAC LoRaWAN a partir do buffer...");
    
    // Restaura os parâmetros salvos de sessão
    int16_t state = node->restoreSession(session_buffer, LORAWAN_SESSION_BUF_SIZE);
    
    if (state == RADIOLIB_ERR_NONE) {
        ESP_LOGI(TAG, "Sessao MAC restaurada com sucesso.");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Falha na restauracao da sessao. Erro RadioLib: %d", state);
        return ESP_FAIL;
    }
}

extern "C" esp_err_t lorawan_node_send(uint8_t f_port, const uint8_t *data, size_t length) {
    if (!node) return ESP_ERR_INVALID_STATE;

    ESP_LOGI(TAG, "Transmitindo payload (%zu bytes, FPort %d)...", length, f_port);
    
    // false ---> Não espera ACK do Gateway
    int16_t state = node->sendReceive((uint8_t*)data, length, f_port, false);

    // O status _NO_DOWNLINK é considerado sucesso no caso de pacotes nao confirmados
    if (state == RADIOLIB_ERR_NONE || state == RADIOLIB_LORAWAN_NO_DOWNLINK) {
        ESP_LOGI(TAG, "Transmissao de Uplink concluida com sucesso.");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Erro na transmissao LoRaWAN: %d", state);
        return ESP_FAIL;
    }
}

extern "C" esp_err_t lorawan_node_sleep(void) {
    if (!radio) return ESP_ERR_INVALID_STATE;
    
    int16_t state = radio->sleep();
    if (state != RADIOLIB_ERR_NONE) {
        ESP_LOGE(TAG, "Falha ao colocar o SX1276 para dormir: %d", state);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "SX1276 dormindo....");
    return ESP_OK;
}