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

#define TEST_I2C_PORT 0
#define I2C_MASTER_SCL_IO 3
#define I2C_MASTER_SDA_IO 1

#define SPI_MOSI_IO 7
#define SPI_MISO_IO 2
#define SPI_SCLK_IO 6
#define SPI_QUADWP_IO -1
#define SPI_QUADHD_IO -1
#define SPI_CS_IO 10

#define EPD_RST_IO 7
#define EPD_DC_IO 5
#define EPD_BUSY_IO 9

#define EPD_RST_PIN 4
#define EPD_BUSY_PIN 19
#define EPD_DC_PIN 5
#define EPD_CS_PIN 10
#define EPD_MOSI_PIN 7
#define EPD_SCK_PIN 6
#define UWORD uint16_t
#define UBYTE uint8_t
#define GPIO_PIN_RESET 0
#define GPIO_PIN_SET 1

#define EPD_4IN2_V2_WIDTH 400
#define EPD_4IN2_V2_HEIGHT 300

#define MCP9808_SENSOR_ADDR                 0x18
#define MCP9808_SENSOR_ADDR_2               0x1C
#define MCP9808_MEASURE_TEMPERATURE         0x05

i2c_master_dev_handle_t MCP9808_dev_handle;
i2c_master_dev_handle_t MCP9808_dev_handle_2;
spi_device_handle_t EPD_dev_handle;

void RTC_IRAM_ATTR esp_wake_deep_sleep(void) {
    esp_default_wake_deep_sleep();
    static RTC_RODATA_ATTR const char fmt_str[] = "Wake From Deep Sleep\n";
    esp_rom_printf(fmt_str);
}

void initial_startup(void) {
    printf("Initial Startup\n");
}

void deep_sleep(void) {
    // Set GPIO pin 0 for wakeup
    esp_deep_sleep_enable_gpio_wakeup(0x01, ESP_GPIO_WAKEUP_GPIO_HIGH);

    // Set sleep duration
    esp_sleep_enable_timer_wakeup(60 * 1000000); // 60 seconds

    printf("Entering Deep Sleep\n");

    // Enter deep sleep
    esp_deep_sleep_start();
}

void GPIO_init(void) {
    // Configure GPIO pin 0 as the inputs to wake up from deep sleep
    gpio_config_t sleep_io_conf = {
        .pin_bit_mask = (1 << GPIO_NUM_0),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&sleep_io_conf);

    // EPD pins
    gpio_config_t epd_o_conf = {
        .pin_bit_mask = (1 << GPIO_NUM_4) | (1 << GPIO_NUM_5) | (1 << GPIO_NUM_6) | (1 << GPIO_NUM_10),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&epd_o_conf);
    gpio_config_t epd_i_conf = {
        .pin_bit_mask = (1 << GPIO_NUM_19),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&epd_i_conf);
}

void i2c_init(void) {
    // Configure the I2C master device
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = TEST_I2C_PORT,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    // Create the I2C master bus
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

    // MCP9808 device configuration
    i2c_device_config_t MCP9808_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MCP9808_SENSOR_ADDR,
        .scl_speed_hz = 100000,
    };

    // MCP9808 device configuration
    i2c_device_config_t MCP9808_cfg_2 = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MCP9808_SENSOR_ADDR_2,
        .scl_speed_hz = 100000,
    };

    // Add devices to I2C bus
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &MCP9808_cfg, &MCP9808_dev_handle));
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &MCP9808_cfg_2, &MCP9808_dev_handle_2));
}

void SPI_init(void) {

    const spi_bus_config_t spi_bus_config = {
        .mosi_io_num = SPI_MOSI_IO,
        .miso_io_num = SPI_MISO_IO,
        .sclk_io_num = SPI_SCLK_IO,
        .quadwp_io_num = SPI_QUADWP_IO,
        .quadhd_io_num = SPI_QUADHD_IO,
    };

    spi_device_interface_config_t EPD_cfg = {
        .clock_speed_hz = 10000000,
        .spics_io_num = SPI_CS_IO,
        .queue_size = 7,
    };

    // Initialize the SPI bus
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &spi_bus_config, SPI_DMA_DISABLED));

    // Add device to SPI bus
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &EPD_cfg, &EPD_dev_handle));
}

void epd_cmd(const uint8_t cmd)
{
    spi_transaction_t EPD_command =  {
        .length = 8,
        .tx_buffer = &cmd,
        .user = 0,
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(EPD_dev_handle, &EPD_command));
}

void epd_data(const uint8_t data)
{
    spi_transaction_t EPD_data =  {
        .length = 8,
        .tx_buffer = &data,
        .user = 0,
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(EPD_dev_handle, &EPD_data));
}

float read_MCP9808(void) {
    // Variable decleration
    float Temp;
    uint8_t data[2];
    uint8_t command = MCP9808_MEASURE_TEMPERATURE;

    // Request and read the temperature data
    ESP_ERROR_CHECK(i2c_master_transmit_receive(MCP9808_dev_handle, &command, sizeof(command), data, sizeof(data), -1));

    // Convert to readable temperature
    // Clear three boundary bits TA<15:13>
    data[0] = data[0] & 0x1F;
    // Determine the sign
    if((data[0] & 0x10) != 0x10) {
        // TA > 0C
        Temp = ((data[0] * 16.0) + (data[1] / 16.0));
    } else {
        // TA <= 0C
        Temp = 256 - ((data[0] * 16.0) + (data[1] / 16.0));
    }

    return Temp;
}

float read_MCP9808_2(void) {
    // Variable decleration
    float Temp;
    uint8_t data[2];
    uint8_t command = MCP9808_MEASURE_TEMPERATURE;

    // Request and read the temperature data
    ESP_ERROR_CHECK(i2c_master_transmit_receive(MCP9808_dev_handle_2, &command, sizeof(command), data, sizeof(data), -1));

    // Convert to readable temperature
    // Clear three boundary bits TA<15:13>
    data[0] = data[0] & 0x1F;
    // Determine the sign
    if((data[0] & 0x10) != 0x10) {
        // TA > 0C
        Temp = ((data[0] * 16.0) + (data[1] / 16.0));
    } else {
        // TA <= 0C
        Temp = 256 - ((data[0] * 16.0) + (data[1] / 16.0));
    }

    return Temp;
}

void app_main(void)
{
    // Get reason for esp32 reset
    esp_reset_reason_t reset_reason = esp_reset_reason();

    // If this is a reset and not a wakeup form deep sleep, run the initial startup
    if (reset_reason == ESP_RST_POWERON) {
        initial_startup();
    } else if (reset_reason == ESP_RST_DEEPSLEEP) {
        // Get reason for wakeup form deep sleep
        esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
        if (wakeup_reason == ESP_SLEEP_WAKEUP_GPIO) {
            printf("GPIO Wakeup\n");
        } else if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
            printf("Timer Wakeup\n");
        }
    }

    // Configure GPIO
    GPIO_init();

    // Initialize i2c
    i2c_init();

    // Initialize SPI
    SPI_init();

    // Read temp from MCP9808
    float Temp;
    Temp = read_MCP9808();

    // Read temp from MCP9808
    float Temp_2;
    Temp_2 = read_MCP9808_2();

    // Print sensor values
    printf("Temperature: %.4f °C\n", Temp);
    printf("Temperature_2: %.4f °C\n", Temp_2);

    // Enter deep sleep
    deep_sleep();
}