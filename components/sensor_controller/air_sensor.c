/*

Definição das funções de mais alto nível para o sensor destinado a medir as variáveis atmosféricas temperatura e umidade.

*/

// Includes
#include "air_sensor.h"
#include "esp_log.h"
#include "dht.h"

// Definição de constantes
static const char *TAG = "air_sensor";
static int sensor_pin = -1; // Variável local para controle do pino de conexão do Sensor de Ar. Alterada apenaspela função de init


esp_err_t air_sensor_init(int pin) {
    if (pin < 0) {
        ESP_LOGE(TAG, "Pino inválido!");
        return ESP_ERR_INVALID_ARG;
    }
    
    sensor_pin = pin;
    ESP_LOGI(TAG, "Sensor de ar configurado no pino GPIO %d", sensor_pin);
    
    return ESP_OK;
}

esp_err_t air_sensor_read(float* temperature, float* humidity) {

    if (temperature == NULL || humidity == NULL) {
        ESP_LOGE(TAG, "Ponteiros inválidos fornecidos para armazenar os dados.");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = dht_read_float_data(DHT_TYPE_AM2301, sensor_pin, humidity, temperature); // DHT_TYPE_AM2301 engloba o DHT22
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao ler dados do sensor DHT22 (%s).", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGD(TAG, "Leitura efetuada: Temp=%.1fC, Hum=%.1f%%", *temperature, *humidity);
    return ESP_OK;
}