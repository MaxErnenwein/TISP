#ifndef __MAIN_H__
#define __MAIN_H__

// Includes
#include <stdio.h>
#include <driver/i2c_master.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
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
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_private/adc_share_hw_ctrl.h"

// Defines
#define GPIO_PIN_RESET  0
#define GPIO_PIN_SET    1

#define COMMAND         0
#define DATA            1

#define FONT8_HEIGHT    8
#define FONT8_WIDTH     58
#define FONT12_HEIGHT   12
#define FONT12_WIDTH    7

#define TEST_I2C_PORT       0
#define I2C_MASTER_SCL_IO   3
#define I2C_MASTER_SDA_IO   1

#define SPI_MOSI_IO     7
#define SPI_MISO_IO     2
#define SPI_SCLK_IO     6
#define SPI_EPD_CS_IO   10
#define SPI_SD_CS_IO    18
#define SPI_QUADWP_IO   -1
#define SPI_QUADHD_IO   -1

#define EPD_RST_PIN         20
#define EPD_BUSY_PIN        19
#define EPD_DC_PIN          5
#define EPD_CS_PIN          10
#define EPD_MOSI_PIN        7
#define EPD_SCK_PIN         6
#define EPD_4IN2_V2_WIDTH   400
#define EPD_4IN2_V2_HEIGHT  300

#define EPD_CMD_WRITE_BW            0x24
#define EPD_CMD_CONF_UPDATE_MODE_2  0x22
#define EPD_CMD_CONF_UPDATE_MODE_1  0x21
#define EPD_CMD_UPDATE_DISPLAY      0x20
#define EPD_CMD_ENTER_DEEP_SLEEP    0x10
#define EPD_CMD_SET_DATA_ENTRY_MODE 0x11
#define EPD_CMD_SET_RAM_X_ADDRESS   0x44
#define EPD_CMD_SET_RAM_Y_ADDRESS   0x45

#define MCP9808_SENSOR_ADDR         0x18
#define MCP9808_SENSOR_ADDR_2       0x1C
#define AHT20_SENSOR_ADDR           0x38
#define VEML7700_SENSOR_ADDR        0x10

#define MCP9808_MEASURE_TEMPERATURE 0x05
#define AHT20_MEADURE_HUMIDITY      0xAC
#define AHT20_MEADURE_HUMIDITY_P1   0x33
#define AHT20_MEADURE_HUMIDITY_P2   0x00
#define VEML7700_MEASURE_LIGHT      0x04
#define VEML7700_CONFIG             0x00
#define VEML7700_CONFIG_P1          0x00
#define VEML7700_CONFIG_P2          0x00

#define DELTA_1_MINUTES     1
#define DELTA_2_MINUTES     2
#define DELTA_5_MINUTES     5
#define DELTA_10_MINUTES    10
#define DELTA_30_MINUTES    30
#define DELTA_60_MINUTES    60
#define DELTA_1_HOURS       DELTA_60_MINUTES
#define DELTA_2_HOURS       120
#define DELTA_5_HOURS       300
#define DELTA_10_HOURS      600
#define NUM_DATA_POINTS     74
#define GRAPH_X_OFFSET      35
#define GRAPH_X_DELTA       5
#define GRAPH_X_LIMIT       400
#define GRAPH_X_HATCH_DELTA 64
#define GRAPH_X_NUM_HATCH   6
#define GRAPH_Y_PIXELS      265
#define GRAPH_Y_OFFSET      10
#define GRAPH_Y_LIMIT       280
#define GRAPH_Y_HATCH_DELTA 27
#define GRAPH_Y_NUM_HATCH   11
#define GRAPH_TEMPERATURE   0
#define GRAPH_HUMIDITY      1
#define GRAPH_LIGHT         2

#define FILE_LOCATION "/sdcard/TISPdata.bin"

// Declare sensor readings struct
struct sensor_readings {
    float temperature;
    unsigned short int humidity;
    unsigned short int light;
    uint32_t time;
};

// Global variables
i2c_master_dev_handle_t MCP9808_dev_handle;
i2c_master_dev_handle_t MCP9808_dev_handle_2;
i2c_master_dev_handle_t AHT20_dev_handle;
i2c_master_dev_handle_t VEML7700_dev_handle;
spi_device_handle_t EPD_dev_handle;
sdmmc_card_t *card;

// Function declerations
void initial_startup(void);
void timer_wakeup_startup(void);
void GPIO_wakeup_startup(void);
void peripherals_init(void);
void GPIO_init(void);
void I2C_init(void);
void SPI_init(void);
void deep_sleep(void);
void EPD_init(void);
void EPD_send_byte(const uint8_t byte, bool dc);
void EPD_busy(void);
void EPD_reset(void);
void EPD_turn_on_display(void);
void EPD_display_image(unsigned char* image);
void EPD_clear(void);
void EPD_deep_sleep(void);
void EPD_set_windows(uint16_t X_start, uint16_t Y_start, uint16_t X_end, uint16_t Y_end);
void EPD_set_cursor(uint16_t X_start, uint16_t Y_start);
void EPD_draw_pixel(uint16_t x, uint16_t y, unsigned char* image);
void EPD_draw_char(uint16_t x, uint16_t y, int font_character_index, int font_size, unsigned char* image);
void EPD_draw_string(uint16_t x, uint16_t y, char* string, int string_size, int font_size, unsigned char* image);
void EPD_draw_line(uint16_t X_start, uint16_t Y_start, uint16_t X_end, uint16_t Y_end, unsigned char* image);
void EPD_draw_graph(int variable, int delta_time, char* file_path, unsigned char* image);
void EPD_draw_sensor_data(void);
void EPD_clear_image(unsigned char* image);
void SD_write_file(char* file_path, struct sensor_readings data);
struct sensor_readings SD_read_file(char* file_path, int index);
float read_MCP9808(void);
float read_MCP9808_2(void);
int read_AHT20(void);
int read_VEML7700(void);

// RTC function declerations
void RTC_IRAM_ATTR esp_wake_deep_sleep(void) {
    esp_default_wake_deep_sleep();
    static RTC_RODATA_ATTR const char fmt_str[] = "Wake From Deep Sleep\n";
    esp_rom_printf(fmt_str);
}

#endif  // __MAIN_H__