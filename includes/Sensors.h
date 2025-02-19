#ifndef SENSORS_H
#define SENSORS_H

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

// Defines
// Sensor commands
#define MCP9808_MEASURE_TEMPERATURE 0x05
#define AHT20_MEADURE_HUMIDITY      0xAC
#define AHT20_MEADURE_HUMIDITY_P1   0x33
#define AHT20_MEADURE_HUMIDITY_P2   0x00
#define VEML7700_MEASURE_LIGHT      0x04
#define VEML7700_CONFIG             0x00
#define VEML7700_CONFIG_P1          0x00
#define VEML7700_CONFIG_P2          0x00
#define C4001_REG_CTRL1             0x02
#define C4001_CHANGE_MODE           0x3B
#define C4001_GET_SENSOR_MODE       0x00
#define C4001_SENSE_MODE            0x81
#define C4001_GET_PRESENCE          0x10

// Function declerations
float read_MCP9808(void);
int read_AHT20(void);
int read_VEML7700(void);
int read_SPW2430(void);
int read_C4001(void);

#endif // SENSORS_H