/*

Arquivo contendo os headers das funções implementadas no arquivo bus.c

*/

// Include guards
#ifndef BUS_H
#define BUS_H

// Includes
#include "esp_log.h"
#include "driver/sdspi_host.h"
#include "driver/i2c.h"
#include "hal/spi_types.h"

// Implementação do protocolo SPI
esp_err_t bus_spi_init(spi_host_device_t host_id, int mosi_pin, int miso_pin, int sclk_pin, int max_transfer_sz);

esp_err_t bus_spi_free(spi_host_device_t host_id);

// Implementação do protocolo I²C
esp_err_t bus_i2c_init(i2c_port_t i2c_num, int sda_pin, int scl_pin, uint32_t clk_speed);

esp_err_t bus_i2c_free(i2c_port_t i2c_num);

#endif 