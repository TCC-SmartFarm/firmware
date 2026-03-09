/*

Arquivo contendo os headers das funções implementadas no arquivo sdcard.c

*/

// Include guards
#ifndef SDCARD_H
#define SDCARD_H

// Includes
#include "esp_err.h"
#include <stddef.h>
#include "hal/spi_types.h"



esp_err_t sdcard_config(spi_host_device_t host_id, int cs_pin, const char* mount_point);

esp_err_t sdcard_write(const char* file_path, const char* data);

esp_err_t sdcard_read_chunk(const char* file_path, size_t offset, char* buffer, size_t buffer_size, size_t* bytes_read);

esp_err_t sdcard_unmount(const char* mount_point);

esp_err_t sdcard_debug_lifecycle(spi_host_device_t host_id, int cs_pin);

#endif 