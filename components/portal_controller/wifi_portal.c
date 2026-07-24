/*

Arquivo destinado a instanciação do portal Wi-Fi para configuração.

*/

// Includes

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include "wifi_portal.h"
#include "device_config.h" // Abriga as infos de configuração

// Tag
static const char *TAG = "wifi_portal";

// Declaração do objeto do servidor
static httpd_handle_t server = NULL;

// HTML para a página do portal
static const char *html_form = 
    "<!DOCTYPE html><html><body>"
    "<h2>Configuracao Datalogger</h2>"
    "<form action=\"/submit\" method=\"POST\">"
    "DevAddr (8 hex): <input type=\"text\" name=\"dev_addr\" maxlength=\"8\"><br><br>"
    "NwkSKey (32 hex): <input type=\"text\" name=\"nwk_s_key\" maxlength=\"32\"><br><br>"
    "AppSKey (32 hex): <input type=\"text\" name=\"app_s_key\" maxlength=\"32\"><br><br>"
    "<input type=\"submit\" value=\"Salvar e Reiniciar\">"
    "</form></body></html>";

// GET Handler
static esp_err_t root_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_form, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// POST Handler -> Extração dos campos e Salvamento
static esp_err_t submit_post_handler(httpd_req_t *req) {
    
    char buf[200];
    int ret, remaining = req->content_len;

    if (remaining >= sizeof(buf)) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Lê o corpo da requisição (URL-encoded)
    ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    // Carrega a configuração atual para não sobrescrever dados não preenchidos
    user_config_t config = {0};
    device_config_load(&config);

    // Extrai os valores do POST
    char param[33];
    if (httpd_query_key_value(buf, "dev_addr", param, sizeof(param)) == ESP_OK) {
        strncpy(config.dev_addr, param, sizeof(config.dev_addr));
    }
    if (httpd_query_key_value(buf, "nwk_s_key", param, sizeof(param)) == ESP_OK) {
        strncpy(config.nwk_s_key, param, sizeof(config.nwk_s_key));
    }
    if (httpd_query_key_value(buf, "app_s_key", param, sizeof(param)) == ESP_OK) {
        strncpy(config.app_s_key, param, sizeof(config.app_s_key));
    }

    config.is_configured = true;

    // Salva na NVS
    if (device_config_save(&config) == ESP_OK) {
        ESP_LOGI(TAG, "Configuracoes salvas via Web.");
        httpd_resp_sendstr(req, "Configuracao salva com sucesso! O dispositivo sera reiniciado.");
        
        // Aguarda 1 segundo para a resposta HTTP ser enviada e reinicia fisicamente o chip
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();

    } else {
        httpd_resp_sendstr(req, "Erro ao salvar na NVS.");
    }

    return ESP_OK;
}

//Configuração das URIs

// Página principal
static const httpd_uri_t root_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_get_handler,
    .user_ctx  = NULL
};

// Confiramção de envio
static const httpd_uri_t submit_uri = {
    .uri       = "/submit",
    .method    = HTTP_POST,
    .handler   = submit_post_handler,
    .user_ctx  = NULL
};

// Inicialização do Access Point
esp_err_t wifi_portal_start(void) {
    ESP_LOGI(TAG, "Inicializando interface Wi-Fi (SoftAP)...");

    // Inicialização padrão do LwIP e Event Loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Parâmetros de Rede
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "DATALOGGER_CONFIG",
            .ssid_len = strlen("DATALOGGER_CONFIG"),
            .channel = 1,
            .password = "", // Sem senha para testes
            .max_connection = 2,
            .authmode = WIFI_AUTH_OPEN
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi AP ativo. Subindo servidor HTTP...");

    httpd_config_t http_config = HTTPD_DEFAULT_CONFIG();
    
    if (httpd_start(&server, &http_config) == ESP_OK) {
        httpd_register_uri_handler(server, &root_uri);
        httpd_register_uri_handler(server, &submit_uri);
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Falha ao iniciar servidor HTTP");
    return ESP_FAIL;
}

// Encerra o servidor
void wifi_portal_stop(void) {
    if (server) {
        httpd_stop(server);
        server = NULL;
    }
    esp_wifi_stop();
    esp_wifi_deinit();
    ESP_LOGI(TAG, "Portal Wi-Fi desativado.");
}