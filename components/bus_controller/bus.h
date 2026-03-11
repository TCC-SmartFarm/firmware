/*

Arquivo contendo os headers das funções implementadas no arquivo bus.c

*/

// Include guards
#ifndef BUS_H
#define BUS_H

// Includes
#include "esp_log.h"
#include "driver/sdspi_host.h"
#include "hal/spi_types.h"

esp_err_t bus_spi_init(spi_host_device_t host_id, int mosi_pin, int miso_pin, int sclk_pin, int max_transfer_sz);

esp_err_t bus_spi_free(spi_host_device_t host_id);

#endif 