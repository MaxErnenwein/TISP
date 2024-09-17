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
#include <string.h>

// Defines
#define GPIO_PIN_RESET  0
#define GPIO_PIN_SET    1
#define COMMAND 0
#define DATA 1

#define TEST_I2C_PORT       0
#define I2C_MASTER_SCL_IO   3
#define I2C_MASTER_SDA_IO   1

#define SPI_MOSI_IO     7
#define SPI_MISO_IO     2
#define SPI_SCLK_IO     6
#define SPI_CS_IO       10
#define SPI_QUADWP_IO   -1
#define SPI_QUADHD_IO   -1

#define EPD_RST_PIN         4
#define EPD_BUSY_PIN        19
#define EPD_DC_PIN          5
#define EPD_CS_PIN          10
#define EPD_MOSI_PIN        7
#define EPD_SCK_PIN         6
#define EPD_4IN2_V2_WIDTH   400
#define EPD_4IN2_V2_HEIGHT  300

#define EPD_CMD_WRITE_BW 0x24
#define EPD_CMD_CONF_UPDATE_MODE_2 0x22
#define EPD_CMD_CONF_UPDATE_MODE_1 0x21
#define EPD_CMD_UPDATE_DISPLAY 0x20
#define EPD_CMD_ENTER_DEEP_SLEEP 0x10
#define EPD_CMD_SET_DATA_ENTRY_MODE 0x11
#define EPD_CMD_SET_RAM_X_ADDRESS 0x44
#define EPD_CMD_SET_RAM_Y_ADDRESS 0x45

#define MCP9808_SENSOR_ADDR         0x18
#define MCP9808_SENSOR_ADDR_2       0x1C
#define MCP9808_MEASURE_TEMPERATURE 0x05

// Global variables
i2c_master_dev_handle_t MCP9808_dev_handle;
i2c_master_dev_handle_t MCP9808_dev_handle_2;
spi_device_handle_t EPD_dev_handle;

// Function declerations
void initial_startup(void);
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
void EPD_draw_pixel(uint16_t x, uint16_t y, unsigned char* image);
void EPD_draw_char(uint16_t x, uint16_t y, int font_character, unsigned char* image);
float read_MCP9808(void);
float read_MCP9808_2(void);

// RTC function declerations
void RTC_IRAM_ATTR esp_wake_deep_sleep(void) {
    esp_default_wake_deep_sleep();
    static RTC_RODATA_ATTR const char fmt_str[] = "Wake From Deep Sleep\n";
    esp_rom_printf(fmt_str);
}

#endif  // __MAIN_H__