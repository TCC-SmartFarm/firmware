/*
Header compartilhado para armazenar a struct de leitura dos sensores. Utilizada tanto pela main quanto pelo payload_formatter
*/

#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include <time.h>
#include <stdbool.h>

// Struct nomeada para permitir forward declaration em outros arquivos
typedef struct sensor_data_t {
    time_t timestamp;     // Unix Epoch
    float air_temp;       // Temperatura do Ar (°C)
    float air_hum;        // Humidade do Ar (%)
    float soil_hum;       // Humidade do Solo (%)
    float light_perc;     // Nível de Luminosidade (%)
    float battery;        // Nível da Bateria (%)
    bool is_valid;        // Flag para indicar se as leituras contêm dados reais ou se falharam
} sensor_data_t;

#endif // SENSOR_DATA_H