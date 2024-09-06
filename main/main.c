#include <stdio.h>
#include <driver/i2c_master.h>

#define TEST_I2C_PORT 0
#define I2C_MASTER_SCL_IO 3
#define I2C_MASTER_SDA_IO 2

#define MCP9808_SENSOR_ADDR                 0x18
#define MCP9808_MEASURE_TEMPERATURE         0x05

i2c_master_dev_handle_t i2c_init()
{
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = TEST_I2C_PORT,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

    i2c_device_config_t MCP9808_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MCP9808_SENSOR_ADDR,
        .scl_speed_hz = 100000,
    };

    i2c_master_dev_handle_t dev_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &MCP9808_cfg, &dev_handle));
    return dev_handle;
}

void app_main(void)
{
    // Initialize i2c
    i2c_master_dev_handle_t dev_handle;
    dev_handle = i2c_init();
}