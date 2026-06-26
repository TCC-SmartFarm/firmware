/*

Header compartilhado para armazenar a struct de leitura dos sensores. Utilizada tanto pela main quanto pelo payload_formatter

*/

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