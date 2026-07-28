
#include "EspHal.h"
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
        // Acopla ao barramento SPI ja inicializado -> Parametro RADIOLIB_NC passado para evitar a re-configuração do barramento SPI
        hal = new EspHal(RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC, (spi_host_device_t)hal_conf->spi_host_id);
        // Instancia um objeto "Module" com os pinos especificados
        mod = new Module(hal, hal_conf->nss_pin, hal_conf->dio0_pin, hal_conf->rst_pin, hal_conf->dio1_pin);
        radio = new SX1276(mod);
        // Frequencia de operação (915MHz) e Sub-Banda 1
        node = new LoRaWANNode(radio, &AU915, 1); 
    }

    // Inicialização do módulo
    ESP_LOGI(TAG, "Inicializando transceptor SX1276 via SPI...");
    int16_t state = radio->begin();
    if (state != RADIOLIB_ERR_NONE) {
        ESP_LOGE(TAG, "Falha na inicializacao do SX1276. Erro RadioLib: %d", state);
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

extern "C" void lorawan_hardware_deinit(void) {
    // Liberação dos recursos
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"

    if (node) { delete node; node = nullptr; }
    if (radio) { delete radio; radio = nullptr; }
    if (mod) { delete mod; mod = nullptr; }

#pragma GCC diagnostic pop

    if (hal) { delete hal; hal = nullptr; }
    ESP_LOGI(TAG, "Recursos de hardware do LoRaWAN liberados.");
}

extern "C" esp_err_t lorawan_activate_abp(const lorawan_keys_t *keys) {
    if (!node || !keys) {
        ESP_LOGE(TAG, "Motor LoRaWAN nao inicializado ou chaves nulas.");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Iniciando ativacao ABP...");
    
    // Passagem as chaves de sessão
    node->beginABP(keys->dev_addr, nullptr, nullptr, (uint8_t*)keys->nwk_s_key, (uint8_t*)keys->app_s_key);
    int16_t state = node->activateABP();
    
    // Validação do estado 
    if (state == RADIOLIB_LORAWAN_NEW_SESSION || state == RADIOLIB_LORAWAN_SESSION_RESTORED || state == RADIOLIB_ERR_NONE) {
        ESP_LOGI(TAG, "Dispositivo ativado via ABP com sucesso. (Codigo de Estado: %d)", state);
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Falha na ativacao ABP. Erro RadioLib: %d", state);
        return ESP_FAIL;
    }

    // Aplica a sessão na biblioteca
    state = node->activateABP();
    
    // O código 2745 (0x0AB9) é o retorno padrão da RadioLib indicando que a sessão ABP está ativa
    if (state == RADIOLIB_ERR_NONE || state == 2745) {
        ESP_LOGI(TAG, "Dispositivo ativado via ABP com sucesso.");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Falha na ativacao ABP. Erro: %d", state);
        return ESP_FAIL;
    }
}

extern "C" esp_err_t lorawan_save_session(uint8_t *session_buffer) {
    if (!node || !session_buffer) return ESP_ERR_INVALID_STATE;

    // Obtém os ponteiros dos buffers internos mantidos pelo RadioLib
    uint8_t *nonces = node->getBufferNonces();
    uint8_t *session = node->getBufferSession();
    
    // Copia os dados particionados para o buffer unificado do projeto
    memcpy(session_buffer, nonces, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
    memcpy(session_buffer + RADIOLIB_LORAWAN_NONCES_BUF_SIZE, session, RADIOLIB_LORAWAN_SESSION_BUF_SIZE);
    
    ESP_LOGI(TAG, "Sessao MAC extraida com sucesso.");
    return ESP_OK;
}

extern "C" esp_err_t lorawan_restore_session(const uint8_t *session_buffer) {
    if (!node || !session_buffer) return ESP_ERR_INVALID_STATE;

    // Restaura os parâmetros salvos de sessão
    ESP_LOGI(TAG, "Restaurando estado MAC LoRaWAN a partir do buffer...");
    // Restaura injetando os dados de volta nos buffers internos
    uint8_t *nonces = node->getBufferNonces();
    uint8_t *session = node->getBufferSession();
    
    memcpy(nonces, session_buffer, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
    memcpy(session, session_buffer + RADIOLIB_LORAWAN_NONCES_BUF_SIZE, RADIOLIB_LORAWAN_SESSION_BUF_SIZE);
    
    ESP_LOGI(TAG, "Sessao MAC restaurada com sucesso.");
    return ESP_OK;
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