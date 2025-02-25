// Includes
#include "../includes/main.h"
#include "../includes/images.h"
#include "../includes/EPD.h"
#include "../includes/SD.h"
#include "../includes/Sensors.h"

extern spi_device_handle_t EPD_dev_handle;
extern sdmmc_card_t *card;
extern i2c_master_dev_handle_t MCP9808_dev_handle;
extern i2c_master_dev_handle_t AHT20_dev_handle;
extern i2c_master_dev_handle_t VEML7700_dev_handle;
extern i2c_master_dev_handle_t C4001_dev_handle;
extern adc_continuous_handle_t SPW2430_dev_handle;
RTC_DATA_ATTR uint32_t settings;

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
            GPIO_wakeup_startup();
        } else if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
            // Check for previous GPIO wakeup that failed
            if ((settings & (1 << SETTING_GPIO_WAKE)) == (1 << SETTING_GPIO_WAKE)) {
                printf("prev GPIO fail\n");
                GPIO_wakeup_startup();
            } else {
                timer_wakeup_startup();
            }
        }
    }

    // Go into deep sleep
    deep_sleep();
}

void initial_startup(void) {
    printf("Initial Startup\n");

    // Go to defualt settings
    settings = 0;

    // Configure peripherals
    GPIO_init();

    // Toggle load switchon
    REG_WRITE(GPIO_OUT_REG, REG_READ(GPIO_OUT_REG) | (1 << LOAD_SWITCH_PIN));

    I2C_init();
    SPI_init();
    ADC_init();

    // Commands and data
    uint8_t eChangeMode[2] = {C4001_REG_CTRL1, C4001_CHANGE_MODE};
    uint8_t readStatus = C4001_GET_SENSOR_MODE;
    uint8_t mode;

    // Put the presence sensor in the correct sensing mode
    do {
        ESP_ERROR_CHECK(i2c_master_transmit(C4001_dev_handle, eChangeMode, sizeof(eChangeMode), -1));
        ESP_ERROR_CHECK(i2c_master_transmit_receive(C4001_dev_handle, &readStatus, sizeof(readStatus), &mode, sizeof(mode), -1));
        vTaskDelay(500 / portTICK_PERIOD_MS);
    } while (mode != C4001_SENSE_MODE);
    
    // Display startup image
    EPD_init();
    EPD_display_image(test_image);
    EPD_deep_sleep();
}

void GPIO_wakeup_startup(void) {
    printf("GPIO Wakeup\n");

    // Signify GPIO wakeup in settings
    settings |= (1 << SETTING_GPIO_WAKE);

    // Configure peripherals
    GPIO_init();

    // Toggle load switch on
    REG_WRITE(GPIO_OUT_REG, REG_READ(GPIO_OUT_REG) | (1 << LOAD_SWITCH_PIN));

    I2C_init();
    SPI_init();
    
    // Initialize the EPD
    EPD_init();

    int graph_state = (settings >> SETTING_GRAPH_STATE_0) & 0xF;
    char *data_file = FILE_LOCATION;
    int ret = 0;

    switch(graph_state) {
        // Draw current sensor data on first push
        case 0:
            ADC_init();
            // Read sound from SPW2430
            int sound;
            sound = read_SPW2430();

            usleep(300000); // Sleep to allow presence sensor time to wakeup
            EPD_draw_sensor_data(current_data_image, sound);
            break;
        case 1: 
            ret = EPD_draw_graph(GRAPH_TEMPERATURE, DELTA_1_MINUTES, data_file, blank_image);
            break;
        case 2: 
            ret = EPD_draw_graph(GRAPH_TEMPERATURE, DELTA_30_MINUTES, data_file, blank_image);
            break;
        case 3: 
            ret = EPD_draw_graph(GRAPH_HUMIDITY, DELTA_1_MINUTES, data_file, blank_image);
            break;
        case 4: 
            ret = EPD_draw_graph(GRAPH_HUMIDITY, DELTA_30_MINUTES, data_file, blank_image);
            break;
        case 5: 
            ret = EPD_draw_graph(GRAPH_LIGHT, DELTA_1_MINUTES, data_file, blank_image);
            break;
        case 6: 
            ret = EPD_draw_graph(GRAPH_LIGHT, DELTA_30_MINUTES, data_file, blank_image);
            break;
        case 7:
            ret = EPD_draw_graph(GRAPH_SOUND, DELTA_1_MINUTES, data_file, blank_image);
            break;
        case 8:
            ret = EPD_draw_graph(GRAPH_PRESENCE, DELTA_1_MINUTES, data_file, blank_image);
            break;
        default:
            break;
    }

    if (ret != 0) {
        EPD_deep_sleep();
        settings |= (1 << SETTING_SD_STATUS);
        esp_sleep_enable_timer_wakeup(1); // Wakeup immediately
        esp_deep_sleep_start();
    }

    // Update to next graph state
    if (graph_state < 8) {
        // Clear graph state bits
        settings &= ~(0xF << SETTING_GRAPH_STATE_0);
        // Add 1 to the graph state
        settings |= ((graph_state + 1) << SETTING_GRAPH_STATE_0);
    } else {
        // Clear graph state
        settings &= ~(0xF << SETTING_GRAPH_STATE_0);
    }

    // Put the EPD to sleep
    EPD_deep_sleep();

    // After successful GPIO runthrough, clear settings bit
    settings &= ~(1 << SETTING_GPIO_WAKE);
}

void timer_wakeup_startup(void) {
    printf("Timer Wakeup\n");

    ADC_init();
    // Read sound from SPW2430 before other peripheral noise
    int sound;
    sound = read_SPW2430();

    // Configure peripherals
    GPIO_init();

    // Turn on load switch
    REG_WRITE(GPIO_OUT_REG, REG_READ(GPIO_OUT_REG) | (1 << LOAD_SWITCH_PIN));

    I2C_init();
    SPI_init();

    // Store current sensor data
    usleep(300000); // Sleep to allow presence sensor time to wakeup
    SD_store_sensor_data(sound);
}

void deep_sleep(void) {
    // Dismount the SD card
    esp_vfs_fat_sdcard_unmount("/sdcard", card);
    sdspi_host_deinit();

    // Get current time
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    
    // Calculate time to next minute
    int64_t sleep_time = (60 - tv_now.tv_sec % 60) * 1000000 - tv_now.tv_usec; 

    // Set GPIO pin 4 for wakeup
    esp_deep_sleep_enable_gpio_wakeup(0x10, ESP_GPIO_WAKEUP_GPIO_HIGH);

    // Set sleep duration
    esp_sleep_enable_timer_wakeup(sleep_time);

    printf("Entering Deep Sleep\n");

    REG_WRITE(GPIO_OUT_REG, REG_READ(GPIO_OUT_REG) & ~(1 << LOAD_SWITCH_PIN));

    // Enter deep sleep
    esp_deep_sleep_start();
}

void GPIO_init(void) {
    // Configure GPIO pin 4 as the inputs to wake up from deep sleep
    gpio_config_t sleep_io_conf = {
        .pin_bit_mask = (1 << GPIO_WAKEUP_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&sleep_io_conf);

    // EPD pins
    gpio_config_t epd_o_conf = {
        .pin_bit_mask = (1 << EPD_DC_PIN) | (1 << EPD_SCK_PIN) | (1 << EPD_CS_PIN) | (1 << EPD_RST_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&epd_o_conf);

    // Configure GPIO19 as the load switch output
    gpio_config_t load_switch_conf = {
        .pin_bit_mask = (1 << LOAD_SWITCH_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&load_switch_conf);
}

void I2C_init(void) {
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

    // AHT20 device configuration
    i2c_device_config_t AHT20_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = AHT20_SENSOR_ADDR,
        .scl_speed_hz = 100000,
    };

    // VEML7700 device configuration
    i2c_device_config_t VEML7700_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = VEML7700_SENSOR_ADDR,
        .scl_speed_hz = 100000,
    };

    // C4001 device configuration
    i2c_device_config_t C4001_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = C4001_SENSOR_ADDR,
        .scl_speed_hz = 100000,
    };

    // Add devices to I2C bus
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &MCP9808_cfg, &MCP9808_dev_handle));
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &AHT20_cfg, &AHT20_dev_handle));
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &VEML7700_cfg, &VEML7700_dev_handle));
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &C4001_cfg, &C4001_dev_handle));

    // Initialize Sensors
    uint8_t command[1] = {AHT20_MEADURE_HUMIDITY};
    ESP_ERROR_CHECK(i2c_master_transmit(AHT20_dev_handle, command, sizeof(command), -1));

    uint8_t config_command[3] = {VEML7700_CONFIG, VEML7700_CONFIG_P1, VEML7700_CONFIG_P2};
    ESP_ERROR_CHECK(i2c_master_transmit(VEML7700_dev_handle, config_command, sizeof(config_command), -1));
}

void SPI_init(void) {
    // Configure the file structure for the SD card
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .max_files = 10,
        .allocation_unit_size = 16 * 1024
    };

    // Set mount point string
    const char mount_point[] = "/sdcard";
    
    // Set SD host struct to default values
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    // Configure SPI bus
    spi_bus_config_t spi_bus_config = {
        .mosi_io_num = SPI_MOSI_IO,
        .miso_io_num = SPI_MISO_IO,
        .sclk_io_num = SPI_SCLK_IO,
        .quadwp_io_num = SPI_QUADWP_IO,
        .quadhd_io_num = SPI_QUADHD_IO,
        .max_transfer_sz = 120000,
    };

    // Configure EPD
    spi_device_interface_config_t EPD_cfg = {
        .clock_speed_hz = 10000000,
        .spics_io_num = SPI_EPD_CS_IO,
        .queue_size = 7,
        .mode = 3
    };

    // Initialize the SPI bus
    ESP_ERROR_CHECK(spi_bus_initialize(host.slot, &spi_bus_config, SDSPI_DEFAULT_DMA));

    // Add device to SPI bus
    ESP_ERROR_CHECK(spi_bus_add_device(host.slot, &EPD_cfg, &EPD_dev_handle));

    // Configure the SD card slot
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SPI_SD_CS_IO;
    slot_config.host_id = host.slot;

    // Mount the SD card
    esp_err_t ret;
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
            // Sleep to retry mounting
            settings |= (1 << SETTING_SD_STATUS);
            esp_sleep_enable_timer_wakeup(1); // Wakeup immediately
            esp_deep_sleep_start();
    }
}

void ADC_init(void) {
    // Set up the hadle configuration
    adc_continuous_handle_cfg_t adc_handle_config = {
        .max_store_buf_size = 1024,
        .conv_frame_size = 100,
    };

    // Create the new handle
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_handle_config, &SPW2430_dev_handle));

    // Setup adc pattern
    adc_digi_pattern_config_t adc_digi_pattern = {
        .atten = ADC_ATTEN_DB_0,
        .channel = ADC_CHANNEL_0,
        .unit = ADC_UNIT_1,
        .bit_width = ADC_BITWIDTH_12,
    };

    // Setup continuous adc
    adc_continuous_config_t adc_config = {
        .pattern_num = 1,
        .adc_pattern = &adc_digi_pattern,
        .sample_freq_hz = 20000, // max = 83333
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
    };

    ESP_ERROR_CHECK(adc_continuous_config(SPW2430_dev_handle, &adc_config));
}
