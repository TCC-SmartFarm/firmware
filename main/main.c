/*

Arquivo contendo o loop principal de execução.

*/

// Includes de sistema
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_sleep.h"

// Includes dos componetes
#include "bus.h"
#include "sdcard.h"
#include "air_sensor.h"
#include "soil_sensor.h"
#include "light_sensor.h"
#include "device_config.h"
#include "serial_cli.h"

static const char *TAG = "main";

/* ------------------------------ Pinagens e Constantes --------------------------------------*/

// DeepSleep
#define SLEEP_DURATION_MIN          0.2 // Tempo que o módulo deverá passar em deepsleep em minutos

// Botão de Menu
#define PIN_NUM_SETUP_BUTTON        33 

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



// Conversor AD externo - ADS1115 (I²C)
#define ADS1115_I2C_ADDRESS         0x48    // Endereço padrão (ADDR ligado em GND)
#define ADS1115_CHANNEL_HIG         0       // Canal A0 -> Higrômetro
#define ADS1115_CHANNEL_LDR         1       // Canal A1 -> LDR

// Cartão SD (SPI)
#define PIN_NUM_SPI_CS_SD        5
#define MOUNT_POINT              "/sdcard"  // Ponto de montagem

// DHT
#define PIN_NUM_SDA_DHT             13

// Módulo Lora


// Struct para armazenar uma leitura
typedef struct {
    time_t timestamp;     // Unix Epoch
    float air_temp;       // Temperatura do Ar (°C)
    float air_hum;        // Humidade do Ar (%)
    float soil_hum;       // Humidade do Solo (%)
    float light_perc;     // Nível de Luminosidade (%)
    bool is_valid;        // Flag para indicar se as leituras contêm dados reais ou se falharam
                          // true -> grava e transmite os dados | false -> Apenas transmite para avisar o estado de erro e não poluir o SD
} sensor_data_t;

/* ------------------------------ Protótipos - Funções de Orquestração --------------------------------------*/

/**
 * @brief Executa o ciclo de configuração via serial ou wifi.
 */
static void execute_user_setup_cycle(void);

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

/**
 * @brief Monta o sistema de arquivos, formata o pacote em CSV e anexa os dados.
 * @param data Ponteiro para a struct contendo os dados do ciclo.
 */
static void save_to_sd_card(const sensor_data_t *data);

/**
 * @brief Liberta os barramentos, desliga periféricos e define como acordar do Deep Sleep.
 */
static void prepare_deep_sleep_and_shutdown(void);

static void test_nvs_storage(void);

/* ------------------------------ Loop Principal - app_main()  --------------------------------------*/

void app_main(void) {

vTaskDelay(pdMS_TO_TICKS(500));
    
    ESP_LOGI(TAG, "\n========== Iniciando! ==========\n");

    // 1. Recursos Críticos
    if (device_config_init() != ESP_OK) {
        ESP_LOGE(TAG, "Falha critica no NVS. Travando dispositivo.");
        while(1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    user_config_t config = {0};
    device_config_load(&config);

    // 2. Avaliação de Estado
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    bool force_configuration = false;

    if (cause == ESP_SLEEP_WAKEUP_EXT0 || !config.is_configured) {
        ESP_LOGI(TAG, "Condicao de configuracao detectada (Botao pressionado ou Sem Config).");
        force_configuration = true;
    }

    // 3. Roteamento
    if (force_configuration) {
        execute_user_setup_cycle();
    } else {
        // 1. Inicializa os barramentos físicos
        if (system_bus_init() != ESP_OK) {
            ESP_LOGE(TAG, "Falha na inicializacao do hardware. Abortando ciclo.");
            return;
        }

        // 2. Executa a leitura em todos os sensores
        sensor_data_t current_data = execute_reading_cycle();

        // 3. Armazena no Cartão SD se os dados forem válidos
        if (current_data.is_valid) {
            save_to_sd_card(&current_data);
        } else {
            ESP_LOGW(TAG, "Leituras invalidas detectadas. Omitindo gravacao no SD para nao poluir o log.");
        }
    }

    ESP_LOGI(TAG, "\n========== Fim do teste! ============\n");

    // 4. Encerramento
    prepare_deep_sleep_and_shutdown();



}


/* ------------------------------ Funções de Orquestração --------------------------------------*/

static void execute_user_setup_cycle(void) {
    ESP_LOGI(TAG, "\n========== Iniciando Ciclo de Configuracao ==========\n");
    ESP_LOGI(TAG, "Inicializando interface UART para teste de presenca...");
    
    if (uart_cli_init() == ESP_OK) { 
        ESP_LOGI(TAG, "Aguardando 5 segundos por atividade na porta Serial...");
        
        bool serial_active = uart_cli_wait_for_user(5000); 

        if (serial_active) {
            ESP_LOGI(TAG, "Atividade detetada. Abrindo Menu de Configuracao.");
            uart_cli_run_menu(); 
        } else {
            ESP_LOGW(TAG, "Timeout atingido sem atividade serial.");
            ESP_LOGI(TAG, "[Simulacao] Iniciando Portal Wi-Fi AP... (Pulado para teste)");
        }
    } else {
        ESP_LOGE(TAG, "Erro ao inicializar o driver UART.");
    }
}

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

    // Timestamp da leitura
    time(&data.timestamp);

    // Delay para estabilização dos sensores
    ESP_LOGI(TAG, "Aguardando estabilização dos sensores (2 segundos)...");
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Leitura do DHT22 
    float air_temp_val = 0, air_hum_val = 0;
    ret_air = air_sensor_read(&air_temp_val, &air_hum_val);
    if (ret_air != ESP_OK) {
        ESP_LOGE(TAG, "Falha na leitura do DHT22.");
    }

    // Acumuladores para as leituras analógicas
    float acc_soil_hum = 0;
    float acc_light_perc = 0;
    int valid_soil_samples = 0;
    int valid_ldr_samples = 0;

    ESP_LOGI(TAG, "Coletando amostras dos sensores analógicos...");
    for (int i = 0; i < NUM_LEITURAS; i++) {
        float sample_soil = 0;
        float sample_light = 0;

        if (soil_sensor_read_ads1115(&sample_soil) == ESP_OK) {
            acc_soil_hum += sample_soil;
            valid_soil_samples++;
        }

        if (light_sensor_read(&sample_light) == ESP_OK) {
            acc_light_perc += sample_light;
            valid_ldr_samples++;
        }

        // Pequeno atraso para o ADC processar a próxima conversão e filtrar ruído AC
        if (i < NUM_LEITURAS - 1) {
            vTaskDelay(pdMS_TO_TICKS(ADC_SAMPLE_DELAY_MS));
        }
    }

    // Validação e calculo das médias
    if (ret_air == ESP_OK && valid_soil_samples > 0 && valid_ldr_samples > 0) {
        
        data.air_temp = air_temp_val;
        data.air_hum = air_hum_val;
        
        // Cálculo da média aritmética
        data.soil_hum = acc_soil_hum / valid_soil_samples;
        data.light_perc = acc_light_perc / valid_ldr_samples;
        
        data.is_valid = true; 

        ESP_LOGI(TAG, "Ciclo concluido com sucesso! (Medias calculadas com base em %d/%d amostras)", 
                 valid_soil_samples, NUM_LEITURAS);
                 
        ESP_LOGI(TAG, "Timestamp - %lld | Valores -> Ar: %.1fC / %.1f%% | Solo: %.1f%% | Luz: %.1f%%", 
                 (long long)data.timestamp, data.air_temp, data.air_hum, data.soil_hum, data.light_perc);
    } else {
        ESP_LOGE(TAG, "Falha critica: Amostras insuficientes para gerar a media dos sensores.");
    }

    return data;
}

static void save_to_sd_card(const sensor_data_t *data){

    // Verificação de integridade
    if (!data->is_valid) {
        ESP_LOGW(TAG, "Dados de sensores inválidos. Gravação no SD abortada.");
        return;
    }

    // Montagem do sistema de arquivo
    ESP_LOGI(TAG, "Montando o cartão SD...");
    if (sdcard_config(SPI_HOST_ID, PIN_NUM_SPI_CS_SD, MOUNT_POINT) != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao acoplar o cartão SD. Abortando armazenamento.");
        return;
    }

    // Montagem da string a ser gravada
    ESP_LOGI(TAG, "Formatando dados em CSV...");
    char csv_buffer[128];
    snprintf(csv_buffer, sizeof(csv_buffer), "%lld,%.2f,%.2f,%.2f,%.2f", 
             (long long)data->timestamp, 
             data->air_temp, 
             data->air_hum, 
             data->soil_hum, 
             data->light_perc);


    // Gravação da linha no arquivo
    ESP_LOGI(TAG, "Gravando linha no arquivo...");
    if (sdcard_write("/sdcard/readings.csv", csv_buffer) != ESP_OK) {
        ESP_LOGE(TAG, "Falha na escrita dos dados.");
    }

    // Desmontagem do sistema de arquivo
    ESP_LOGI(TAG, "Desmontando o cartão SD...");
    sdcard_unmount(MOUNT_POINT);
}

static void prepare_deep_sleep_and_shutdown(void) {
    ESP_LOGI(TAG, "Iniciando Tear Down do sistema...");

    //Liberação dos barramentos
    bus_i2c_free(I2C_MASTER_NUM);
    bus_spi_free(SPI_HOST_ID);

    // Definindo o despertador
    const uint64_t wakeup_time_sec = SLEEP_DURATION_MIN * 60; // Minutos para segundos
    esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000ULL); // Segundos para microssegundos
    ESP_LOGI(TAG, "Despertador configurado para %i minutos.", SLEEP_DURATION_MIN);

    // Definindo a fonte externa de Wake-Up (Botão ---> Menu de configuração)
    esp_sleep_enable_ext0_wakeup(PIN_NUM_SETUP_BUTTON, 1); 


    ESP_LOGI(TAG, "Dromindo...");
    
    // Delay para registro da mensagem
    vTaskDelay(pdMS_TO_TICKS(100)); 

    // Entra em deep sleep
    esp_deep_sleep_start();
}

 






