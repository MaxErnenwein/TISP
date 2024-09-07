#include <stdio.h>
#include <driver/i2c_master.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TEST_I2C_PORT 0
#define I2C_MASTER_SCL_IO 3
#define I2C_MASTER_SDA_IO 2

#define MCP9808_SENSOR_ADDR                 0x18
#define MCP9808_SENSOR_ADDR_2               0x1C
#define MCP9808_MEASURE_TEMPERATURE         0x05

#define SI7021_SENSOR_ADDR                  0x40
#define SI7021_MEADURE_HUMIDITY             0xF5

i2c_master_dev_handle_t MCP9808_dev_handle;
i2c_master_dev_handle_t MCP9808_dev_handle_2;
i2c_master_dev_handle_t SI7021_dev_handle;

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

    // SI7021 device configuration
    i2c_device_config_t SI7021_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SI7021_SENSOR_ADDR,
        .scl_speed_hz = 100000,
    };

    // Add devices to I2C bus
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &MCP9808_cfg, &MCP9808_dev_handle));
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &MCP9808_cfg_2, &MCP9808_dev_handle_2));
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &SI7021_cfg, &SI7021_dev_handle));
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

float read_SI7021(void)
{
    // Variable decleration
    float Humidity = 0;
    uint8_t data[2] = {0,0};
    uint8_t command = SI7021_MEADURE_HUMIDITY;

    // Request the humidity data
    ESP_ERROR_CHECK(i2c_master_transmit(SI7021_dev_handle, &command, sizeof(command), -1));

    // Delay so sensor can convert to RH
    vTaskDelay(15 / portTICK_PERIOD_MS);

    // Read the humidity data
    ESP_ERROR_CHECK(i2c_master_receive(SI7021_dev_handle, data, sizeof(data), -1));
    
    // Convert to readable humidity
    Humidity = ((125 * ((data[0] << 8) | data[1])) / 65536) - 6;
    
    return Humidity;
}

void app_main(void)
{
    // Initialize i2c
    i2c_init();

    // Read temp from MCP9808
    float Temp;
    Temp = read_MCP9808();

    // Read temp from MCP9808
    float Temp_2;
    Temp_2 = read_MCP9808_2();

    // Read humidity from SI7021
    float Humidity;
    Humidity = read_SI7021();

    printf("Temperature: %.4f °C\n", Temp);
    printf("Temperature_2: %.4f °C\n", Temp_2);
    printf("Humidity: %f%%\n", Humidity);
}