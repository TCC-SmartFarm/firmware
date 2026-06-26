#include "payload_formatter.h"
#include "sensor_data.h" 

void lora_payload_formatter(const struct sensor_data_t *data, uint8_t *payload_buffer) {
    if (!data || !payload_buffer) return;

    // Timestamp (4 bytes - uint32_t, Big-Endian)
    uint32_t ts = (uint32_t)data->timestamp;
    payload_buffer[0] = (ts >> 24) & 0xFF;
    payload_buffer[1] = (ts >> 16) & 0xFF;
    payload_buffer[2] = (ts >> 8) & 0xFF;
    payload_buffer[3] = ts & 0xFF;

    // Temperatura do Ar (2 bytes - int16_t * 100, suporta negativos)
    int16_t temp = (int16_t)(data->air_temp * 100.0f);
    payload_buffer[4] = (temp >> 8) & 0xFF;
    payload_buffer[5] = temp & 0xFF;

    // Umidade do Ar (2 bytes - uint16_t)
    uint16_t air_hum = (uint16_t)(data->air_hum * 100.0f);
    payload_buffer[6] = (air_hum >> 8) & 0xFF;
    payload_buffer[7] = air_hum & 0xFF;

    // Umidade do Solo (2 bytes - uint16_t)
    uint16_t soil_hum = (uint16_t)(data->soil_hum * 100.0f);
    payload_buffer[8] = (soil_hum >> 8) & 0xFF;
    payload_buffer[9] = soil_hum & 0xFF;

    // Luminosidade (2 bytes - uint16_t)
    uint16_t light = (uint16_t)(data->light_perc * 100.0f);
    payload_buffer[10] = (light >> 8) & 0xFF;
    payload_buffer[11] = light & 0xFF;
}