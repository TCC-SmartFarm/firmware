/*

Arquivo contendo o loop principal de execução.

*/

// Includes de sistema
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Includes dos componetes
#include "bus.h"
#include "sdcard.h"
#include "air_sensor.h"
#include "soil_sensor.h"
#include "light_sensor.h"

static const char *TAG = "main";

/* ------------------------------ Pinagens e Constantes --------------------------------------*/


// Barramento I²C
#define PIN_NUM_I2C_SCL             22
#define PIN_NUM_I2C_SDA             21
#define I2C_MASTER_NUM              I2C_NUM_0  // Interface I2C Zero do ESP32
#define I2C_FREQ_HZ                 400000    // Alterar este valor para mudar o modo de operação (Fast Mode = 400kHZ)


// Barramento SPI
#define SPI_HOST_ID              SPI2_HOST
#define PIN_NUM_SPI_MISO         19
#define PIN_NUM_SPI_MOSI         23
#define PIN_NUM_SPI_CLK          18
#define SPI_MAX_TRANSFER         4000



// Conversor AD externo - ADS1115
#define ADS1115_I2C_ADDRESS         0x48    // Endereço padrão (ADDR ligado em GND)
#define ADS1115_CHANNEL_HIG         0       // Canal A0 -> Higrômetro
#define ADS1115_CHANNEL_LDR         1       // Canal A1 -> LDR

// Cartão SD
#define PIN_NUM_SPI_CS_SD        5


// DHT
#define PIN_NUM_SDA_DHT             13

// Módulo Lora


// Struct para armazenar uma leitura
typedef struct {
    float air_temp;       // Temperatura do Ar (°C)
    float air_hum;        // Humidade do Ar (%)
    float soil_hum;       // Humidade do Solo (%)
    float light_perc;     // Nível de Luminosidade (%)
    bool is_valid;        // Flag para indicar se as leituras contêm dados reais ou se falharam
                          // true -> grava e transmite os dados | false -> Apenas transmite para avisar o estado de erro e não poluir o SD
} sensor_data_t;

/* ------------------------------ Protótipos - Funções de Orquestração --------------------------------------*/

/**
 * @brief Inicializa a infraestrutura de barramentos (SPI e I2C) do sistema.
 * @return esp_err_t ESP_OK se todos os barramentos foram inicializados com sucesso.
 * Retorna o código de erro específico caso algum barramento falhe.
 */

static esp_err_t system_bus_init(void);

/**
 * @brief Instancia os drivers dos sensores, efetua uma leitura e agrupa os resultados.
 * @return sensor_data_t Struct contendo os valores numéricos e uma flag de erro ou sucesso das leituras.
 */
static sensor_data_t execute_reading_cycle(void);

void app_main(void) {
    ESP_LOGI(TAG, "\n========== Inicializado! ============\n");




    ESP_LOGI(TAG, "\n========== Fim do teste! ============\n");
}


/* ------------------------------ Funções de Orquestração --------------------------------------*/

static esp_err_t system_bus_init(void) {

    ESP_LOGI(TAG, "\n========== Inicializando Barramentos... ============\n");
    esp_err_t ret;

    // SPI
    ret = bus_spi_init(SPI_HOST_ID, PIN_NUM_SPI_MOSI, PIN_NUM_SPI_MISO, PIN_NUM_SPI_CLK, SPI_MAX_TRANSFER);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha critica: Nao foi possivel inicializar o barramento SPI.");
        return ret;
    }

    // I2C
    ret = bus_i2c_init(I2C_MASTER_NUM, PIN_NUM_I2C_SDA, PIN_NUM_I2C_SCL, I2C_FREQ_HZ);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha critica: Nao foi possivel inicializar o barramento I2C.");
        
        // Liberação do barramento em caso de falha
        bus_spi_free(SPI_HOST_ID);
        
        return ret;
    }

    // Ok!
    ESP_LOGI(TAG, "Todos os barramentos inicializados com sucesso.");
    return ESP_OK;
}

static sensor_data_t execute_reading_cycle(void) {
    ESP_LOGI(TAG, "Iniciando ciclo de aquisição de dados...");
    
    // Inicialização da struct
    sensor_data_t data = {0}; 
    data.is_valid = false;

    // Inicialização dos sensores
    esp_err_t ret_air = air_sensor_init(PIN_NUM_SDA_DHT);
    esp_err_t ret_soil = soil_sensor_init_ads1115(I2C_MASTER_NUM, ADS1115_I2C_ADDRESS, ADS1115_CHANNEL_HIG);
    esp_err_t ret_ldr = light_sensor_init(I2C_MASTER_NUM, ADS1115_I2C_ADDRESS, ADS1115_CHANNEL_LDR);

    // Verificação da inicialização
    if (ret_air != ESP_OK || ret_soil != ESP_OK || ret_ldr != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao instanciar os drivers dos sensores. Abortando leitura.");
        return data; // is_valid = false
    }

    // Delay para estabilização dos sensores
    ESP_LOGI(TAG, "Aguardando estabilização dos sensores (2 segundos)...");
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Leitura
    ESP_LOGI(TAG, "Coletando amostras...");
    ret_air = air_sensor_read(&data.air_temp, &data.air_hum);
    ret_soil = soil_sensor_read_ads1115(&data.soil_hum);
    ret_ldr = light_sensor_read(&data.light_perc);

    // Validação
    if (ret_air == ESP_OK && ret_soil == ESP_OK && ret_ldr == ESP_OK) {
        data.is_valid = true; // Indicativo de uma leitura bem sucedida
        ESP_LOGI(TAG, "Ciclo concluído com sucesso!");
        ESP_LOGI(TAG, "Valores -> Ar: %.1fC / %.1f%% | Solo: %.1f%% | Luz: %.1f%%", 
                 data.air_temp, data.air_hum, data.soil_hum, data.light_perc);
    } else {
        ESP_LOGE(TAG, "Falha de comunicação em um ou mais sensores durante a leitura.");
        // is_valid = false
    }

    return data;
}