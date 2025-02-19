#ifndef SD_H
#define SD_H

// Includes
#include <stdio.h>
#include <driver/i2c_master.h>
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "hal/spi_types.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <float.h>
#include "driver/rtc_io.h"
#include <sys/time.h>
#include "esp_adc/adc_continuous.h"
#include <unistd.h>
#include "esp_rom_gpio.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_sig_map.h"
#include "../includes/main.h"

// Defines
// Data file name
#define FILE_LOCATION "/sdcard/TISPdata.bin"

// Function declerations
void SD_write_file(char* file_path, struct sensor_readings data);
void SD_store_sensor_data(int sound);
struct sensor_readings SD_read_file(char* file_path, int index);

#endif // SD_H