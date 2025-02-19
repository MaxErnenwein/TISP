#ifndef EPD_H
#define EPD_H

// Includes
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <limits.h>
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
#include <float.h>
#include "driver/rtc_io.h"
#include <sys/time.h>
#include "esp_adc/adc_continuous.h"
#include <unistd.h>
#include "esp_rom_gpio.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_sig_map.h"

// Defines
#define GPIO_PIN_RESET  0
#define GPIO_PIN_SET    1

// E-Paper DC bit
#define COMMAND         0
#define DATA            1

// Font Sizes
#define FONT8_HEIGHT    8
#define FONT8_WIDTH     5
#define FONT12_HEIGHT   12
#define FONT12_WIDTH    7
#define FONT24_HEIGHT   24
#define FONT24_WIDTH    17

// Setting bits
#define SETTING_MCP9808_STATUS  6

// EPD GPIO pins
#define EPD_RST_PIN         20
//#define EPD_BUSY_PIN        19
#define EPD_DC_PIN          5
#define EPD_CS_PIN          10
#define EPD_MOSI_PIN        7
#define EPD_SCK_PIN         6

// EPD dimensions
#define EPD_4IN2_V2_WIDTH   400
#define EPD_4IN2_V2_HEIGHT  300

// EPD commnads
#define EPD_CMD_WRITE_BW            0x24
#define EPD_CMD_CONF_UPDATE_MODE_2  0x22
#define EPD_CMD_CONF_UPDATE_MODE_1  0x21
#define EPD_CMD_UPDATE_DISPLAY      0x20
#define EPD_CMD_ENTER_DEEP_SLEEP    0x10
#define EPD_CMD_SET_DATA_ENTRY_MODE 0x11
#define EPD_CMD_SET_RAM_X_ADDRESS   0x44
#define EPD_CMD_SET_RAM_Y_ADDRESS   0x45
#define EPD_CMD_SET_RAM_X_ADDRESS_COUNTER   0x4E
#define EPD_CMD_SET_RAM_Y_ADDRESS_COUNTER   0x4F

// Graph Formatting
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

// Function declerations
void EPD_init(void);
void EPD_set_windows(uint16_t X_start, uint16_t Y_start, uint16_t X_end, uint16_t Y_end);
void EPD_set_cursor(uint16_t X_start, uint16_t Y_start);
void EPD_turn_on_display(void);
void EPD_reset(void);
void EPD_clear(void);
void EPD_clear_image(unsigned char* image);
void EPD_send_byte(const uint8_t byte, bool dc);
void EPD_draw_pixel(uint16_t x, uint16_t y, unsigned char* image);
void EPD_draw_char(uint16_t x, uint16_t y, int font_character_index, int font_size, unsigned char* image);
void EPD_draw_string(uint16_t x, uint16_t y, char* string, int string_size, int font_size, unsigned char* image);
void EPD_draw_line(uint16_t X_start, uint16_t Y_start, uint16_t X_end, uint16_t Y_end, unsigned char* image);
void EPD_draw_sensor_data(unsigned char* image, int sound);
void EPD_display_image(unsigned char* image);
int EPD_draw_graph(int variable, int delta_time, char* file_path, unsigned char* image);
void EPD_deep_sleep(void);

#endif // EPD_H