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
        .max_transfer_sz = 15000,
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

void EPD_send_byte(const uint8_t byte, bool dc) {
    // Set d/c pin if the byte is data or a command
    if (dc == COMMAND) {
        gpio_set_level(EPD_DC_PIN, COMMAND);
    } else {
        gpio_set_level(EPD_DC_PIN, DATA);
    }

    // Create the SPI transaction
    spi_transaction_t EPD_byte =  {
        .length = 8,
        .tx_buffer = &byte,
    };

    // Send command
    ESP_ERROR_CHECK(spi_device_transmit(EPD_dev_handle, &EPD_byte));
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

void EPD_busy(void) {
    printf("EPD Busy\n");
    // Wait until the EPD is not busy
    while(gpio_get_level(EPD_BUSY_PIN) == 1);
    printf("EPD Not Busy\n");
}

void EPD_reset(void) {
    // Set reset pin low
    gpio_set_level(EPD_RST_PIN, GPIO_PIN_RESET);
    // Delay 200us
    vTaskDelay(2 / portTICK_PERIOD_MS);
    // Set reset pin high
    gpio_set_level(EPD_RST_PIN, GPIO_PIN_SET);
    // Delay 200us
    vTaskDelay(2 / portTICK_PERIOD_MS);
    // Wait for display
    EPD_busy(); 
}

static void EPD_turn_on_display(void) {
    // Set update mode
    EPD_send_byte(EPD_CMD_CONF_UPDATE_MODE_2, COMMAND);
	EPD_send_byte(0xF7, DATA);
    // Update display
    EPD_send_byte(EPD_CMD_UPDATE_DISPLAY, COMMAND);
    EPD_busy();
}

void EPD_deep_sleep(void) {
    // Put the display into deep sleep
    EPD_send_byte(EPD_CMD_ENTER_DEEP_SLEEP, COMMAND);
    EPD_send_byte(0x01, DATA);
}

static void EPD_set_windows(uint16_t X_start, uint16_t Y_start, uint16_t X_end, uint16_t Y_end) {
    // Set RAM start and end X position
    EPD_send_byte(EPD_CMD_SET_RAM_X_ADDRESS, COMMAND);
    EPD_send_byte((X_start >> 3) & 0xFF, DATA); // Divide by 8 since x direction is divided into bytes
    EPD_send_byte((X_end >> 3) & 0xFF, DATA);
	
    // Set RAM start and end y position
    EPD_send_byte(EPD_CMD_SET_RAM_Y_ADDRESS, COMMAND);
    EPD_send_byte(Y_start & 0xFF, DATA); // y direction is divided into bits, so two bytes need to be sent
    EPD_send_byte((Y_start >> 8) & 0xFF, DATA);
    EPD_send_byte(Y_end & 0xFF, DATA);
    EPD_send_byte((Y_end >> 8) & 0xFF, DATA);
}

void EPD_init(void) {
    // Reset the display
    EPD_reset();

    // Configure update mode
    EPD_send_byte(EPD_CMD_CONF_UPDATE_MODE_1, COMMAND);
    EPD_send_byte(0x40, DATA);
	
    // Set data entry mode to x address
    EPD_send_byte(EPD_CMD_SET_DATA_ENTRY_MODE, COMMAND);
    EPD_send_byte(0x03, DATA);
		
	EPD_set_windows(0, 0, EPD_4IN2_V2_WIDTH-1, EPD_4IN2_V2_HEIGHT-1);
	 
    EPD_busy();
}

void EPD_clear(void) {
    // Calculate width and height
    uint16_t Width = EPD_4IN2_V2_WIDTH / 8; // Divide by 8 since bytes are being sent
    uint16_t Height = EPD_4IN2_V2_HEIGHT;

    // Write to screen in BW
    EPD_send_byte(EPD_CMD_WRITE_BW, COMMAND);
    // Set each pixel to 1 (black)
    for (int i = 0; i < Height*Width; i++) {
        EPD_send_byte(0xFF, DATA);
    }

    // Turn on the display
    EPD_turn_on_display();
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

    EPD_init();
    EPD_clear();
    EPD_deep_sleep();

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