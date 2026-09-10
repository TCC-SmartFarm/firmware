/*

Arquivo destinado a apresentar o menu de configuração no terminal para o usuário. Assume que o usuário esteja conectado ao ESP32 com um cabo serial

*/

// Includes
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "device_config.h"
#include "serial_cli.h"

// Defines
#define EX_UART_NUM UART_NUM_0
#define BUF_SIZE (256)

// Constantes
static const char *TAG = "serial_cli";
static user_config_t temp_config = {0}; // Ver o arquivo ---> device_config/device_config.h

typedef enum {
    STATE_MAIN_MENU,
    STATE_WAIT_MAIN_MENU_INPUT,
    STATE_SUBMENU_INPUT
} cli_state_t; // Maquina de estados para o menu de configuração

// Definição do estado inicial
static cli_state_t estado_atual = STATE_MAIN_MENU;
static int submenu_atual = -1; 

static void print_main_menu(void) {

    // Exibição das opções no terminal
    printf("\n-+H+-+H+-+H+-+H+-+H+-+H+-+H+-+H+-+H+-\n");
    printf("        MENU DE CONFIGURACAO      \n");
    printf("-+H+-+H+-+H+-+H+-+H+-+H+-+H+-+H+-+H+-\n \n");
    printf("[1] DevEUI                   : %s\n", temp_config.dev_addr[0] ? temp_config.dev_addr : "(vazio)");
    printf("[2] Network Session Key      : %s\n", temp_config.nwk_s_key[0] ? temp_config.nwk_s_key : "(vazio)");
    printf("[3] Application Session Key  : %s\n", temp_config.app_s_key[0] ? temp_config.app_s_key : "(vazio)");
    printf("[4] Data e Hora Atual        : %lu\n",temp_config.setup_date);
    printf("[7] Apagar Configuracoes da Memoria (Reset NVS)\n");
    printf("[8] Ver Configuracoes Atuais \n");
    printf("[9] Salvar e Sair\n");
    printf("==================================\n");
    printf("Escolha uma opcao: ");
    fflush(stdout);
}

static void print_current_flash_config(void) {
    // Mostra as configurações já existentes na memória flash

    // Extrai as informações da memória flash
    user_config_t flash_cfg = {0};

    // Exibe no terminal
    if (device_config_load(&flash_cfg) == ESP_OK) {
        printf("\n-+H+- DADOS GRAVADOS NA MEMORIA -+H+-\n");
        printf("DevEUI                  : %s\n", flash_cfg.dev_addr);
        printf("Network Session Key     : %s\n", flash_cfg.nwk_s_key);
        printf("Application Session Key : %s\n", flash_cfg.app_s_key);
        printf("Data e Hora             : %lu\n", flash_cfg.setup_date);
        printf("-+H+-+H+-+H+-+H+-+H+-+H+-+H+-+H+-+H+-\n");
    } else {
        printf("\n[!] Nenhuma configuracao previa encontrada na memoria.\n");
    }
}

// Configuração da interface UART
esp_err_t uart_cli_init(void) {

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    }; // Parâmetros de configuração
    
    esp_err_t ret = uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
    if (ret != ESP_OK){
        return ret;
    } // Instalação do Driver

    ret = uart_param_config(EX_UART_NUM, &uart_config);
    if (ret != ESP_OK){
        return ret;
    } // Configuração da interface

    return uart_set_pin(EX_UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

// Timeout para esperar o usuário
bool uart_cli_wait_for_user(uint32_t timeout_ms) {

    uint8_t dummy_buf[1];
    // Aguarda um único byte até o limite do timeout informado
    int len = uart_read_bytes(EX_UART_NUM, dummy_buf, 1, pdMS_TO_TICKS(timeout_ms));
    return (len > 0);
}

void uart_cli_run_menu(void) {
    // Pré configurações
    uint8_t data[BUF_SIZE];
    char input_buffer[BUF_SIZE];
    int input_pos = 0;

    // Carregamento da configuração prévia
    device_config_load(&temp_config);

    // Loop principal
    while (1) {
        // Definição do estado inicial
        if (estado_atual == STATE_MAIN_MENU) {
            print_main_menu();
            estado_atual = STATE_WAIT_MAIN_MENU_INPUT;
        }
        
        // Prepara para ler o próximo caractere na serial
        int len = uart_read_bytes(EX_UART_NUM, data, BUF_SIZE - 1, pdMS_TO_TICKS(20));

        // Tratativa de input ---> Backspace e delete
        if (len > 0) {
            for (int i = 0; i < len; i++) {
                char c = (char)data[i];

                if (c == '\b' || c == 0x7F) {
                    if (input_pos > 0) {
                        input_pos--;
                        printf("\b \b");
                        fflush(stdout);
                    }
                }

                // Tratativa de input ---> Seleção da opção
                else if (c == '\r' || c == '\n') {
                    input_buffer[input_pos] = '\0'; 
                    
                    if (estado_atual == STATE_WAIT_MAIN_MENU_INPUT) {
                        if (input_pos > 0) {
                            int option = input_buffer[0] - '0';

                            // Opções dos submenus
                            if (option >= 1 && option <= 4) {
                                submenu_atual = option;
                                estado_atual = STATE_SUBMENU_INPUT;
                                if (option == 4) {
                                    printf("\n \n> Insira a data e hora (AAAA-MM-DD HH:mm): ");
                                } else {
                                    printf("\n \n> Insira o novo valor (Enter para confirmar): ");
                                }
                                fflush(stdout); // Flush od buffer para exibição imediata dos submenus

                            // Limpeza da memória
                            } else if (option == 7) {
                                printf("\n[!] Apagando configuracoes do NVS e limpando a Flash...\n");
                                fflush(stdout);
                                
                                // Executa o reset lógico e físico do namespace
                                if (device_config_reset() == ESP_OK) {
                                    printf("Sucesso. O dispositivo sera reiniciado no modo de fabrica.\n");
                                } else {
                                    printf("Erro ao limpar a particao NVS.\n");
                                }
                                fflush(stdout);
                                vTaskDelay(pdMS_TO_TICKS(1000));
                                esp_restart(); // Reinicia para forçar o fluxo de configuração no próximo boot

                            // Exibição das configurações pré-existentes    
                            } else if (option == 8) {
                                print_current_flash_config();
                                estado_atual = STATE_MAIN_MENU;

                            // Salvar e Sair
                            } else if (option == 9) {
                                printf("\n \nSalvando...\n");
                                temp_config.is_configured = true;
                                device_config_save(&temp_config);
                                printf("\n Configuracao salva com sucesso! O dispositivo sera reiniciado.\n");
                                vTaskDelay(pdMS_TO_TICKS(1000));
                                
                                // Reinicialização do ESP ja configurado
                                esp_restart(); 

                            // Tratativa de input ---> Input Inválido
                            } else {
                                printf("\n \n[!] Opcao invalida.\n");
                                estado_atual = STATE_MAIN_MENU;
                            }
                        }

                    // Configuração dos Submenus
                    } else if (estado_atual == STATE_SUBMENU_INPUT) {
                        if (input_pos > 0) {

                             // Submenu ---> DevEUI
                            if (submenu_atual == 1) {
                                strncpy(temp_config.dev_addr, input_buffer, sizeof(temp_config.dev_addr) - 1);
                                temp_config.dev_addr[sizeof(temp_config.dev_addr) - 1] = '\0';
                                printf("\n[+] Entrada recebida!\n");

                             // Submenu ---> Network Session Key
                            } else if (submenu_atual == 2) {
                                strncpy(temp_config.nwk_s_key, input_buffer, sizeof(temp_config.nwk_s_key) - 1);
                                temp_config.nwk_s_key[sizeof(temp_config.nwk_s_key) - 1] = '\0';
                                printf("\n[+] Entrada recebida!\n");

                             // Submenu ---> Application Session Key
                            } else if (submenu_atual == 3) {
                                strncpy(temp_config.app_s_key, input_buffer, sizeof(temp_config.app_s_key) - 1);
                                temp_config.app_s_key[sizeof(temp_config.app_s_key) - 1] = '\0';
                                printf("\n[+] Entrada recebida!\n");
                            
                             // Submenu ---> Data e Hora (conversão para Epoch)
                            } else if (submenu_atual == 6) {
                                int ano, mes, dia, hora, min;
                                if (sscanf(input_buffer, "%d-%d-%d %d:%d", &ano, &mes, &dia, &hora, &min) == 5) {
                                    struct tm data_atual = {0};
                                    data_atual.tm_year = ano - 1900; 
                                    data_atual.tm_mon = mes - 1;    
                                    data_atual.tm_mday = dia;
                                    data_atual.tm_hour = hora;
                                    data_atual.tm_min = min;
                                    data_atual.tm_sec = 0;
                                    data_atual.tm_isdst = -1;         
                                    
                                    // Conversão para Epoch usando a biblioteca time
                                    time_t epoch = mktime(&data_atual);
                                    
                                    // Ajuste do relógio interno
                                    if (epoch != -1) {
                                        temp_config.setup_date = (uint32_t)epoch;
                                        struct timeval now = { .tv_sec = epoch, .tv_usec = 0 };
                                        settimeofday(&now, NULL);
                                        printf("\n \n[+] Epoch gerado (%lu) e relogio sincronizado.\n", temp_config.setup_date);
                                    } else {
                                        printf("\n \n[!] Erro na conversao da data.\n");
                                    }
                                } else {
                                    printf("\n \n[!] Formato invalido. A operacao foi cancelada.\n");
                                }
                            }

                        // Tratativa de entrada inválida
                        } else {
                            printf("\n \n[!] Operacao cancelada (entrada vazia).\n");
                        }

                        // Retorno ao menu princiapl
                        submenu_atual = -1;
                        estado_atual = STATE_MAIN_MENU;
                    }

                    // Retorno do carro para a posição inicial
                    input_pos = 0; 
                }
                
                
                // Registro do input
                else if (input_pos < BUF_SIZE - 1) {
                    input_buffer[input_pos++] = c;
                    putchar(c); 
                    fflush(stdout);
                }
            }
        }
        
        // Passa o controle momentâneo para o RTOS evitar disparo do Watchdog
        vTaskDelay(pdMS_TO_TICKS(10));
    }

}