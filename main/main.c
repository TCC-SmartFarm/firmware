/*

Arquivo contendo o loop principal de execução.

*/

// Includes de sistema
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_attr.h"

// Includes dos componentes
#include "bus.h"
#include "sdcard.h"
#include "air_sensor.h"
#include "soil_sensor.h"
#include "light_sensor.h"
#include "sensor_data.h"
#include "device_config.h"
#include "serial_cli.h"
#include "lorawan_config.h"
#include "payload_formatter.h"
#include "wifi_portal.h"

static const char *TAG = "main";

/* ------------------------------ Pinagens e Constantes --------------------------------------*/

// DeepSleep
#define SLEEP_DURATION_MIN          0.2 // Tempo que o módulo deverá passar em deepsleep em minutos

// Botão de Menu
#define PIN_NUM_SETUP_BUTTON        35 

// Barramento I²C
#define PIN_NUM_I2C_SCL             22
#define PIN_NUM_I2C_SDA             21
#define I2C_MASTER_NUM              I2C_NUM_0  // Interface I2C Zero do ESP32
#define I2C_FREQ_HZ                 400000    // Alterar este valor para mudar o modo de operação (Fast Mode = 400kHZ)

// Barramento SPI - Cartão SD
#define SPI_HOST_ID              SPI2_HOST
#define PIN_NUM_SPI_MISO_SD      19
#define PIN_NUM_SPI_MOSI         23
#define PIN_NUM_SPI_CLK          18
#define SPI_MAX_TRANSFER         4000

// Cartão SD (SPI)
#define PIN_NUM_SPI_CS_SD        12
#define MOUNT_POINT              "/sdcard"  // Ponto de montagem
#define MAX_FILE_SIZE_BYTES      1024       // ----------------- TESTE: 1 KB para forçar a rotação rápida
#define MAX_LOG_FILES            5          // ----------------- TESTE: 5 arquivos no máximo

// Barramento SPI - Módulo LoRa
#define SPI_HOST_ID              SPI2_HOST
#define PIN_NUM_SPI_MISO_LORA    15
#define PIN_NUM_SPI_MOSI         23
#define PIN_NUM_SPI_CLK          18
#define SPI_MAX_TRANSFER         4000

// Módulo Lora
#define PIN_NUM_CS_LORA             26      // Chip select (NSS) -> LoRa
#define PIN_NUM_RST_LORA            25      // Pino para resetar o módulo
#define PIN_NUM_DIO0_LORA           32      // Controle de Tx e Rx
#define PIN_NUM_DIO1_LORA           33      // Controle de Tx e Rx
#define LORAWAN_SESSION_BUF_SIZE    256     // Tamanho do buffer (denifino em lorawan_config.h)
#define NVS_BACKUP_INTERVAL         50      // Backup na Flash a cada 50 transmissões

// Memória RTC
RTC_DATA_ATTR static bool rtc_lora_session_valid = false;                       // Verificador da existencia de uma sessão (p/ cold boot i.e)
RTC_DATA_ATTR static uint8_t rtc_lora_session_buffer[LORAWAN_SESSION_BUF_SIZE]; // Buffer para estado de sessão lora
RTC_DATA_ATTR static uint32_t rtc_uplink_counter = 0;                           // Contador para backup

// Conversor AD externo - ADS1115 (I²C)
#define ADS1115_I2C_ADDRESS         0x48    // Endereço padrão (ADDR ligado em GND)
#define ADS1115_CHANNEL_HIG         0       // Canal A0 -> Higrômetro
#define ADS1115_CHANNEL_LDR         1       // Canal A1 -> LDR
                                            // Amostragem:
#define NUM_READINGS                5       //  Número de amostras de sensores analógicos
#define ADC_SAMPLE_DELAY_MS         20      //  Delay entre as amostras

// DHT
#define PIN_NUM_SDA_DHT             32



/* ------------------------------ Protótipos - Funções de Orquestração --------------------------------------*/

/**
 * @brief Executa o ciclo de configuração via serial ou wifi.
 */
static void execute_user_setup_cycle(void);

/**
 * @brief Inicializa a infraestrutura do barramentos I2C do sistema.
 * @return esp_err_t ESP_OK se o barramento foi inicializado com sucesso.
 * Retorna o código de erro específico caso o barramento falhe.
 */

static esp_err_t i2c_init(void);

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
 * @brief Autentica na rede LoRa, formata os dados coletados e transmite.
 * @param data Ponteiro para a struct contendo os dados a serem transmitidos.
 * @param config Ponteiro para a struct contendo os parametros persisnteste de conexão.
 */
static void execute_transmission_cycle(const sensor_data_t *data, const user_config_t *config);

/**
 * @brief Liberta os barramentos, desliga periféricos e define como acordar do Deep Sleep.
 */
static void prepare_deep_sleep_and_shutdown(void);

/* ------------------------------ Protótipos - Funções Auxiliares --------------------------------------*/


/* ------------------------------ Loop Principal - app_main()  --------------------------------------*/

void app_main(void) {

vTaskDelay(pdMS_TO_TICKS(1000)); // Aguarda estabilização da serial
    ESP_LOGI(TAG, "\n========== Iniciando Datalogger (Teste ABP/Transmissao) ==========\n");

    // Instancia NVS
    if (device_config_init() != ESP_OK) {
        ESP_LOGE(TAG, "Falha critica no NVS. Travando dispositivo.");
        while(1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    user_config_t config = {0};
    device_config_load(&config);

    gpio_set_direction((gpio_num_t)PIN_NUM_CS_LORA, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)PIN_NUM_CS_LORA, 1);

    gpio_set_direction(PIN_NUM_SPI_CS_SD, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_NUM_SPI_CS_SD, 1);

    // Resolução do Wake Up
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    bool force_configuration = true;

    if (cause == ESP_SLEEP_WAKEUP_EXT0 || !config.is_configured) {
        ESP_LOGI(TAG, "Condicao de configuracao detectada (Botao ou Falta de Credenciais).");
        force_configuration = true;
    }

    if (force_configuration) {
        // Desvia para o fluxo de CLI e trava aqui até conclusão/reboot
        execute_user_setup_cycle(); 
    } else {
        // Fluxo operacional normal
        if (i2c_init() == ESP_OK) {
            
            // Coleta de Dados
            sensor_data_t data = execute_reading_cycle();
            
            //Dados dummy em caso de falha de leitura
            if (!data.is_valid) {
                ESP_LOGW(TAG, "Sensores falharam. Injetando dados DUMMY para testar transmissao LoRa.");
                data.air_temp = 25.5;
                data.air_hum = 60.0;
                data.soil_hum = 45.0;
                data.light_perc = 80.0;
                data.battery = 100.0;
                time(&data.timestamp);
                data.is_valid = true;
            }

            // Armazenamento
            save_to_sd_card(&data);

            // Transmissão
            execute_transmission_cycle(&data, &config);
        } else {
            ESP_LOGE(TAG, "Falha na inicializacao do hardware. Abortando ciclo operacional.");
        }
    }

    // Encerramento
    prepare_deep_sleep_and_shutdown();
}


/* ------------------------------ Funções de Orquestração --------------------------------------*/

static void execute_user_setup_cycle(void) {
    ESP_LOGI(TAG, "\n========== Iniciando Ciclo de Configuracao ==========\n");
    if (uart_cli_init() == ESP_OK) { 
        ESP_LOGI(TAG, "Aguardando 5 segundos por atividade na porta Serial...");
        if (uart_cli_wait_for_user(5000)) {
            ESP_LOGI(TAG, "Atividade detectada. Abrindo Menu de Configuracao.");
            uart_cli_run_menu(); 
        } else {
            ESP_LOGW(TAG, "Timeout serial atingido. Levantando Portal Wi-Fi.");
            
            // Inicia o Access Point e o Servidor Web
            if (wifi_portal_start() == ESP_OK) {
                ESP_LOGI(TAG, "Portal Web ativo. Conecte-se a rede e acesse o IP de configuracao.");
                
                // Trava a execução nesta task indefinidamente. 
                // O encerramento deste ciclo se dará via hard reset (esp_restart) 
                // acionado internamente pelo callback do POST HTTP.
                while(1) {
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
            } else {
                ESP_LOGE(TAG, "Falha ao iniciar portal Wi-Fi. Reiniciando...");
                vTaskDelay(pdMS_TO_TICKS(1000));
                esp_restart();
            }
        }
    }
}

static esp_err_t i2c_init(void) {

    ESP_LOGI(TAG, "\n========== Inicializando Barramento I²C... ============\n");
    esp_err_t ret;

    // I2C
    ret = bus_i2c_init(I2C_MASTER_NUM, PIN_NUM_I2C_SDA, PIN_NUM_I2C_SCL, I2C_FREQ_HZ);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha critica: Nao foi possivel inicializar o barramento I2C.");
        
        // Liberação do barramento em caso de falha
        bus_i2c_free(I2C_MASTER_NUM);
        
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
    for (int i = 0; i < NUM_READINGS; i++) {
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
        if (i < NUM_READINGS - 1) {
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
                 valid_soil_samples, NUM_READINGS);
                 
        ESP_LOGI(TAG, "Timestamp - %lld | Valores -> Ar: %.1fC / %.1f%% | Solo: %.1f%% | Luz: %.1f%%", 
                 (long long)data.timestamp, data.air_temp, data.air_hum, data.soil_hum, data.light_perc);
    } else {
        ESP_LOGE(TAG, "Falha critica: Amostras insuficientes para gerar a media dos sensores.");
    }

    return data;
}

static void save_to_sd_card(const sensor_data_t *data) {

    // Inicialização do barramento SPI para o cartão SD
    ESP_LOGI(TAG, "\n========== Inicializando Barramento SPI (SD)... ============\n");
    esp_err_t ret;

    // SPI
    ret = bus_spi_init(SPI_HOST_ID, PIN_NUM_SPI_MOSI, PIN_NUM_SPI_MISO_SD, PIN_NUM_SPI_CLK, SPI_MAX_TRANSFER);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha critica: Nao foi possivel inicializar o barramento SPI.");
    }

    // Verifica se o dado é válido
    if (!data->is_valid) return;

    // Configura o cartão SD
    if (sdcard_config(SPI_HOST_ID, PIN_NUM_SPI_CS_SD, MOUNT_POINT) != ESP_OK) {
        return;
    }

    // Tenta acessar o sistema de arquivos
    DIR *dir = opendir(MOUNT_POINT);
    if (!dir) {
        ESP_LOGE(TAG, "Falha ao abrir diretorio do SD.");
        return;
    }

    // Variáveis para busca do log mais antigo
    struct dirent *entry;
    long long latest_timestamp = -1;
    long long oldest_timestamp = -1;
    char oldest_filename[64] = {0};
    int file_count = 0;

    // Busca para achar o log mais antigo
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "log_", 4) == 0) {
            file_count++;
            long long ts = 0;
            if (sscanf(entry->d_name, "log_%lld.csv", &ts) == 1) {
                if (ts > latest_timestamp) latest_timestamp = ts;
                if (oldest_timestamp == -1 || ts < oldest_timestamp) {
                    oldest_timestamp = ts;
                    strncpy(oldest_filename, entry->d_name, sizeof(oldest_filename) - 1);
                }
            }
        }
    }
    closedir(dir);

    // Exlcusão caso o armazenamento esteja cheio
    if (file_count >= MAX_LOG_FILES && oldest_timestamp != -1) {
        char path_to_delete[128];
        snprintf(path_to_delete, sizeof(path_to_delete), "%s/%s", MOUNT_POINT, oldest_filename);
        ESP_LOGW(TAG, "Limite de arquivos atingido (%d). Apagando o mais antigo: %s", MAX_LOG_FILES, path_to_delete);
        unlink(path_to_delete);
    }

    // Rotação de arquivos
    bool create_new_file = false;
    char current_file_path[128];
    
    if (latest_timestamp == -1) {
        create_new_file = true; // Primeiro arquivo do cartão SD
    } else {
        snprintf(current_file_path, sizeof(current_file_path), "%s/log_%lld.csv", MOUNT_POINT, latest_timestamp);
        struct stat st;
        if (stat(current_file_path, &st) == 0) {
            if (st.st_size >= MAX_FILE_SIZE_BYTES) {
                create_new_file = true;
                ESP_LOGI(TAG, "Arquivo atual atingiu o limite de tamanho. Rotacionando...");
            }
        }
    }

    // Atualiza o timestamp se precisar criar arquivo novo
    if (create_new_file) {
        latest_timestamp = (long long)data->timestamp;
        snprintf(current_file_path, sizeof(current_file_path), "%s/log_%lld.csv", MOUNT_POINT, latest_timestamp);
    }

    // Gravação da leitura
    char csv_buffer[128];
    snprintf(csv_buffer, sizeof(csv_buffer), "%lld,%.2f,%.2f,%.2f,%.2f\n", 
             (long long)data->timestamp, data->air_temp, data->air_hum, data->soil_hum, data->light_perc);

    if (sdcard_write(current_file_path, csv_buffer) != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao gravar no arquivo %s", current_file_path);
    } else {
        ESP_LOGI(TAG, "Dados gravados em: %s", current_file_path);
    }

    // Liberação dos recursos
    sdcard_deinit(MOUNT_POINT);

    // Liberação do Barramento SPI para o cartão SD
    bus_spi_free(SPI_HOST_ID);
}

static void execute_transmission_cycle(const sensor_data_t *data, const user_config_t *config) {

    // Inicialização do barramento SPI para o módulo LoRa
    ESP_LOGI(TAG, "\n========== Inicializando Barramento SPI (LoRa)... ============\n");
    esp_err_t ret;
    
    // SPI
    ret = bus_spi_init(SPI_HOST_ID, PIN_NUM_SPI_MOSI, PIN_NUM_SPI_MISO_LORA, PIN_NUM_SPI_CLK, SPI_MAX_TRANSFER);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha critica: Nao foi possivel inicializar o barramento SPI.");
    }

    ESP_LOGI(TAG, "\n========== Iniciando Transmissao LoRaWAN ==========\n");


    // Validação de dados e configuração
    if (!data->is_valid) {
        ESP_LOGE(TAG, "Dados invalidos. Abortando transmissao.");
        return;
    }
    if (!config->is_configured) {
        ESP_LOGE(TAG, "Dispositivo nao configurado. Abortando transmissao.");
        return;
    }

    // Acoplamento de Hardware
    lorawan_hal_config_t hal_conf = {
        .spi_host_id = SPI_HOST_ID,
        .nss_pin = PIN_NUM_CS_LORA,
        .rst_pin = PIN_NUM_RST_LORA,
        .dio0_pin = PIN_NUM_DIO0_LORA,
        .dio1_pin = PIN_NUM_DIO1_LORA
    };

    if (lorawan_hardware_init(&hal_conf) != ESP_OK) {
        ESP_LOGE(TAG, "Falha na inicializacao fisica do radio.");
        return;
    }

    // Mapeamento de Chaves ABP
    lorawan_keys_t keys = {0};
    keys.dev_addr = (uint32_t)strtoul(config->dev_addr, NULL, 16);
    // Converte as strings de 32 caracteres do NwkSKey e AppSKey para arrays de 16 bytes
    for (int i = 0; i < 16; i++) {
        // Escaneia 2 caracteres hex por vez (%2hhx) e salva no respectivo byte (uint8_t)
        sscanf(&config->nwk_s_key[i * 2], "%2hhx", &keys.nwk_s_key[i]);
        sscanf(&config->app_s_key[i * 2], "%2hhx", &keys.app_s_key[i]);
    }

    // Ativação na Rede
    if (lorawan_activate_abp(&keys) != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao ativar dispositivo na rede via ABP.");
        lorawan_hardware_deinit();
        return;
    }

    // Formatação do Payload
    uint8_t payload[LORAWAN_PAYLOAD_SIZE] = {0};
    lora_payload_formatter(data, payload);
    
    ESP_LOGI(TAG, "Payload formatado (%d bytes). Enviando para a porta 1...", LORAWAN_PAYLOAD_SIZE);

    // Transmissão (Uplink FPort 1)
    if (lorawan_node_send(1, payload, sizeof(payload)) != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao despachar o pacote LoRaWAN.");
    }

    // Liberação dos recursos
    lorawan_node_sleep(); // Repouso do rádio

    // Desacoplamento para liberação do barramento
    lorawan_hardware_deinit();

    // Liberação do barramento SPI para o módulo LoRa
    bus_spi_free(SPI_HOST_ID);
    
    ESP_LOGI(TAG, "Ciclo de transmissao encerrado.");
}

static void prepare_deep_sleep_and_shutdown(void) {
    ESP_LOGI(TAG, "Iniciando Tear Down do sistema...");

    //Liberação do barramento I2C
    bus_i2c_free(I2C_MASTER_NUM);
    

    // Definindo o despertador
    const uint64_t wakeup_time_sec = SLEEP_DURATION_MIN * 60; // Minutos para segundos
    esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000ULL); // Segundos para microssegundos
    ESP_LOGI(TAG, "Despertador configurado para %i minutos.", SLEEP_DURATION_MIN);

    // Definindo a fonte externa de Wake-Up (Botão ---> Menu de configuração)
    esp_sleep_enable_ext0_wakeup(PIN_NUM_SETUP_BUTTON, 1); 


    ESP_LOGI(TAG, "Dormindo...");
    
    // Delay para registro da mensagem
    vTaskDelay(pdMS_TO_TICKS(100)); 

    // Entra em deep sleep
    esp_deep_sleep_start();
}

/* --------------------------------- Funções Auxiliares ----------------------------------------*/



