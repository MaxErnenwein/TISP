#include <stdio.h>
#include <driver/i2c_master.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "driver/gpio.h"

#define TEST_I2C_PORT 0
#define I2C_MASTER_SCL_IO 3
#define I2C_MASTER_SDA_IO 2

#define MCP9808_SENSOR_ADDR                 0x18
#define MCP9808_SENSOR_ADDR_2               0x1C
#define MCP9808_MEASURE_TEMPERATURE         0x05

i2c_master_dev_handle_t MCP9808_dev_handle;
i2c_master_dev_handle_t MCP9808_dev_handle_2;

/*void WiFi_Bluetooth_deinit(void)
{
    nimble_port_stop();
    nimble_port_deinit();
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
    esp_wifi_stop();
}*/

void GPIO_init(void)
{
    // Configure GPIO pins 0 and 1 as inputs to wake up from deep sleep
    gpio_config_t io_conf = {
        .pin_bit_mask = (1 << GPIO_NUM_0) | (1 << GPIO_NUM_1),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&io_conf);
}

void i2c_init(void)
{
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

float read_MCP9808(void)
{
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

float read_MCP9808_2(void)
{
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
    // Configure GPIO
    GPIO_init();

    // Initialize i2c
    i2c_init();

    // Read temp from MCP9808
    float Temp;
    Temp = read_MCP9808();

    // Read temp from MCP9808
    float Temp_2;
    Temp_2 = read_MCP9808_2();

    // Print sensor values
    printf("Temperature: %.4f °C\n", Temp);
    printf("Temperature_2: %.4f °C\n", Temp_2);

    esp_deep_sleep_enable_gpio_wakeup(0x33, ESP_GPIO_WAKEUP_GPIO_HIGH);

    // Set sleep duration (optional, for time-based wakeup)
    esp_sleep_enable_timer_wakeup(10 * 1000000); // 10 seconds

    // Enter deep sleep
    esp_deep_sleep_start();
}