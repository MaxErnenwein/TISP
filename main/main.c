// Includes
#include "../includes/main.h"
#include "../includes/images.h"
#include "../includes/fonts.h"

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
            timer_wakeup_startup();
        }
    }

    // Enter deep sleep
    deep_sleep();
}

void EPD_draw_graph(int variable, int delta_time, char* file_path, unsigned char* image) {
    // Open the data in SD card
    FILE* file = fopen(file_path, "rb");

    // Check for file open error
    if (file == NULL) {
        printf("File failed to open\n");
        return;
    }

    struct sensor_readings data = {0};
    struct sensor_readings empty_struct = {0};
    int graph_data_int[NUM_DATA_POINTS];
    int largest_int = INT_MIN;
    int smallest_int = INT_MAX;
    float graph_data_flt[NUM_DATA_POINTS];
    float largest_flt = INT_MIN;
    float smallest_flt = INT_MAX;
    int points_cnt = 76;


    // Go to last data measurement
    fseek(file, -1 * (sizeof(struct sensor_readings)), SEEK_END);

    // Read data needed from file
    for (int i = 0; i < points_cnt; i++) {
        
        // Read data from file
        int read = fread(&data, sizeof(struct sensor_readings), 1, file);
        if (read != 1) {
            printf("Failed to read file data point %d\n", i);
            data = empty_struct;
            points_cnt--;
        }

        // Put desired data in array
        switch (variable) {
            case 0: 
                graph_data_flt[i] = data.temperature;
                break;
            case 1: 
                graph_data_int[i] = data.humidity;
                break;
            case 2: 
                graph_data_int[i] = data.light;
                break;
            default: 
                return;
                break;
        }

        // Find the largest and smallest values in the data
        if (variable < 1) {
            if (graph_data_flt[i] > largest_flt) {
                largest_flt = graph_data_flt[i];
            } else if (graph_data_flt[i] < smallest_flt) {
                smallest_flt = graph_data_flt[i];
            }
        } else {
            if (graph_data_int[i] > largest_int) {
                largest_int = graph_data_int[i];
            } else if (graph_data_int[i] < smallest_int) {
                smallest_int = graph_data_int[i];
            }
        }

        // Go to previous data in file
        fseek(file, -2 * (delta_time * sizeof(struct sensor_readings)), SEEK_CUR);
    }

    // Find the change of one pixel in the y direction
    float delta_y_pixel;
    if (variable < 1) {
        delta_y_pixel = ((float)(largest_flt - smallest_flt))/GRAPH_Y_PIXELS;
    } else {
        delta_y_pixel = ((float)(largest_int - smallest_int))/GRAPH_Y_PIXELS;
    }


    // Draw the lines in the graph
    int X_start, X_end, Y_start, Y_end;
    for (int i = 0; i < points_cnt; i++) {
        X_start = 20 + (i * 5);
        X_end = 25 + (i * 5);
        if (variable < 1) {
            Y_start = (int)(((float)(largest_flt - graph_data_flt[i]))/delta_y_pixel);
            Y_end = (int)(((float)(largest_flt - graph_data_flt[i + 1]))/delta_y_pixel);
        } else {
            Y_start = (int)(((float)(largest_int - graph_data_int[i]))/delta_y_pixel);
            Y_end = (int)(((float)(largest_int - graph_data_int[i + 1]))/delta_y_pixel);
        }
        
        EPD_draw_line(X_start, Y_start, X_end, Y_end, image);
    }

    // Display the graph
    EPD_display_image(image);
}


void SD_write_file(char* file_path, struct sensor_readings data) {
    // Open file to be written to 
    FILE* file = fopen(file_path, "ab");

    // Check for file open error
    if (file == NULL) {
        printf("File failed to open\n");
        return;
    }

    // Write data to file
    int written = fwrite(&data, sizeof(data), 1, file);

    // Check if data was written
    if (written != 1) {
        printf("Error writing to file");
    }

    // Close the file
    fclose(file);
}

struct sensor_readings SD_read_file(char* file_path, int index) {
    // Open the file to be read
    FILE* file = fopen(file_path, "rb");

    struct sensor_readings data = {0};

    // Check for file open error
    if (file == NULL) {
        printf("File failed to open\n");
        return data;
    }

    // Get the offset of data requested
    long file_offset = (long)(index * sizeof(struct sensor_readings));
    
    // Seek to requested data
    int seek = fseek(file, file_offset, SEEK_SET);
    if (seek != 0) {
        printf("Failed to seek file");
        fclose(file);
        return data;
    }

    // Read data from file
    int read = fread(&data, sizeof(struct sensor_readings), 1, file);
    if (read != 1) {
        printf("Failed to read file");
    }
    
    // Close the file
    fclose(file);

    return data;
}

void SD_store_sensor_data(void) {
    // Get sensor values
    struct sensor_readings data = {
        .temperature = read_MCP9808(),
        .humidity = read_AHT20(),
        .light = read_VEML7700(),
        .time = time(NULL)
    };

    // Create or write to file
    char *data_file = FILE_LOCATION;
    SD_write_file(data_file, data);
}

void EPD_draw_pixel(uint16_t x, uint16_t y, unsigned char* image) {
    // Check to see if pixel is on screen
    if (x > EPD_4IN2_V2_WIDTH || y > EPD_4IN2_V2_HEIGHT) {
        //printf("pixel out of screen range\n");
        return;
    }
    // Get byte number in image
    int byte = (y * 50 + (x >> 3));
    // Draw to pixel in that byte
    image[byte] &= ~(1 << (7 - (x % 8)));
}

void EPD_draw_char(uint16_t x, uint16_t y, int font_character_index, int font_size, unsigned char* image) {
    // Get the width of the font
    int font_width;
    switch(font_size) {
        case 8:
            font_width = FONT8_WIDTH;
            break;
        case 12:
            font_width = FONT12_WIDTH;
            break;
        default: 
            font_width = 0;
            break;
    }

    // Loop through each byte in the font
    for (int i = 0; i < font_size; i++) {
        // Loop through each pixel in the width of the font
        for (int j = 0; j < font_width; j++) {
            // Determine what font table to use
            if(font_size == 8) {
                // Check if pixel shoudl be drawn
                if((((Font8_Table[font_character_index + i]) >> (7 - j)) & 0x01) == 0x01) {
                    EPD_draw_pixel(x + j, y + i, image);
                }
            } else if (font_size == 12) {
                // Check if pixel shoudl be drawn
                if((((Font12_Table[font_character_index + i]) >> (7 - j)) & 0x01) == 0x01) {
                    EPD_draw_pixel(x + j, y + i, image);
                }
            }
            
        }
    }
}

void EPD_draw_string(uint16_t x, uint16_t y, char* string, int string_size, int font_size, unsigned char* image) {
    // Variables
    char character;
    int index;

    // Get the width of the font
    int font_width;
    switch(font_size) {
        case 8:
            font_width = FONT8_WIDTH;
            break;
        case 12:
            font_width = FONT12_WIDTH;
            break;
        default: 
            font_width = 0;
            break;
    }

    // Print each character in the passed string
    for (int i = 0; i <= string_size - 2; i++) {
        // Get the next character
        character = string[i];
        // Calculate index into font table
        index = (((int)character) - 32) * font_size;
        // Draw the character
        EPD_draw_char(x + i*font_width + i, y, index, font_size, image);
    }
}

void EPD_draw_line(uint16_t X_start, uint16_t Y_start, uint16_t X_end, uint16_t Y_end, unsigned char* image) {
    // Calculate the change in x and y directions
    float X_diff;
    float Y_diff;
    X_diff = (X_end - X_start);
    Y_diff = (Y_end - Y_start);
    int greater; // The larger change from start to end
    int lesser; // The smaller change from start to end
    float ratio; // The ratio of smaller/larger change
    int direction = abs((int)X_diff) >= abs((int)Y_diff); // Determine what direction is the main (larger) one

    // Set variables depending on the main direction
    if (direction) {
        greater = X_diff;
        lesser = Y_diff;
        ratio = Y_diff / X_diff;
    } else {
        greater = Y_diff;
        lesser = X_diff;
        ratio = X_diff / Y_diff;
    }

    // Find the sign for the direction of change
    int main_sign = (greater >= 0) ? 1 : -1;
    int secondary_sign = (lesser >= 0) ? 1 : -1;

    // Keep the ratio positive
    if(ratio < 0) {
        ratio *= -1;
    }

    // Draw the line
    for (int i = 0; i < abs((int)greater) + 1; i++) {
        // Depending on if x is the main or secondary direction, switch function
        if (direction) {
            EPD_draw_pixel(X_start + (i * main_sign), Y_start + (int)(i * ratio * secondary_sign), image);
        } else {
            EPD_draw_pixel(X_start + (int)(i * ratio * secondary_sign), Y_start + (i * main_sign), image);
        }
    }
}

void initial_startup(void) {
    printf("Initial Startup\n");

    // Configure peripherals
    GPIO_init();
    I2C_init();
    SPI_init();

    //EPD_draw_sensor_data();

    // Display startup image
    EPD_init();
    EPD_display_image(butterfly_image);
    EPD_deep_sleep();
}

void GPIO_wakeup_startup(void) {
    printf("GPIO Wakeup\n");

    // Configure peripherals
    GPIO_init();
    I2C_init();
    SPI_init();

    // Store current sensor data
    SD_store_sensor_data();

    // Draw graph
    char *data_file = FILE_LOCATION;

    // Display current sensor data
    EPD_init();
    EPD_draw_graph(GRAPH_TEMPERATURE, DELTA_1_MINUTES, data_file, current_data_image);
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    EPD_clear_image(current_data_image);
    EPD_draw_graph(GRAPH_TEMPERATURE, DELTA_2_MINUTES, data_file, current_data_image);
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    EPD_clear_image(current_data_image);
    EPD_draw_graph(GRAPH_TEMPERATURE, DELTA_5_MINUTES, data_file, current_data_image);
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    EPD_clear_image(current_data_image);
    EPD_draw_graph(GRAPH_HUMIDITY, DELTA_1_MINUTES, data_file, current_data_image);
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    EPD_clear_image(current_data_image);
    EPD_draw_graph(GRAPH_LIGHT, DELTA_1_MINUTES, data_file, current_data_image);
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    EPD_deep_sleep();
}

void timer_wakeup_startup(void) {
    printf("Timer Wakeup\n");

    // Configure peripherals
    GPIO_init();
    I2C_init();
    SPI_init();

    // Store current sensor data
    SD_store_sensor_data();

    EPD_init();
    EPD_deep_sleep();
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

    // MCP9808 device configuration
    i2c_device_config_t MCP9808_cfg_2 = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MCP9808_SENSOR_ADDR_2,
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

    // Add devices to I2C bus
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &MCP9808_cfg, &MCP9808_dev_handle));
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &MCP9808_cfg_2, &MCP9808_dev_handle_2));
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &AHT20_cfg, &AHT20_dev_handle));
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &VEML7700_cfg, &VEML7700_dev_handle));
}

void SPI_init(void) {
    // Configure the file structure for the SD card
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .max_files = 10,
        .allocation_unit_size = 16 * 1024
    };

    // Declare variable for the SD card
    sdmmc_card_t *card;

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
        .max_transfer_sz = 4000,
    };

    // Configure EPD
    spi_device_interface_config_t EPD_cfg = {
        .clock_speed_hz = 10000000,
        .spics_io_num = SPI_EPD_CS_IO,
        .queue_size = 7,
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
    ESP_ERROR_CHECK(esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card));

    // Print SD card information
    //sdmmc_card_print_info(stdout, card);
}

void EPD_draw_sensor_data(void) {
    // Read temp from MCP9808
    float Temp;
    Temp = read_MCP9808();

    // Read temp from MCP9808
    float Temp_2;
    Temp_2 = read_MCP9808_2();

     // Read temp from MCP9808
    int humidity;
    humidity = read_AHT20();

    int lux;
    lux = read_VEML7700();

    // Declare strings
    char temp_string[20];
    char temp_string_2[22];
    int humidity_string_size;
    int lux_string_size;
    if (humidity < 10) {
        humidity_string_size = 13;
    } else {
        humidity_string_size = 14;
    }
    char humidity_string[humidity_string_size];
    if (lux < 10) {
        lux_string_size = 19;
    } else  if (lux < 100) {
        lux_string_size = 20;
    } else  if (lux < 1000) {
        lux_string_size = 21;
    } else if (lux < 10000) {
        lux_string_size = 22;
    } else {
        lux_string_size = 23;
    }
    char lux_string[lux_string_size];    

    // Convert temperature value to string
    snprintf(temp_string, sizeof(temp_string), "Temperature: %.2fC", Temp);
    snprintf(temp_string_2, sizeof(temp_string_2), "Temperature 2: %.2fC", Temp_2);
    snprintf(humidity_string, sizeof(humidity_string), "Humidity: %d%%", humidity);
    snprintf(lux_string, sizeof(lux_string), "Light level: %d lux", lux);

    // Draw temperature values to display
    EPD_draw_string(0, 0, temp_string, sizeof(temp_string), FONT12_HEIGHT, current_data_image);
    EPD_draw_string(0, 14, temp_string_2, sizeof(temp_string_2), FONT12_HEIGHT, current_data_image);
    EPD_draw_string(0, 28, humidity_string, sizeof(humidity_string), FONT12_HEIGHT, current_data_image);
    EPD_draw_string(0, 42, lux_string, sizeof(lux_string), FONT12_HEIGHT, current_data_image);
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

int read_AHT20(void) {
    // Variable declaration
    uint32_t humidity = 0;
    uint8_t command[3] = {AHT20_MEADURE_HUMIDITY, AHT20_MEADURE_HUMIDITY_P1, AHT20_MEADURE_HUMIDITY_P2};
    uint8_t data[7];
    
    // Request and read the humidity data
    ESP_ERROR_CHECK(i2c_master_transmit(AHT20_dev_handle, command, sizeof(command), -1));
    ESP_ERROR_CHECK(i2c_master_receive(AHT20_dev_handle, data, sizeof(data), -1));
        
    // Calculate relative humidity
    humidity |= data[1];
    humidity = humidity << 8;
    humidity |= data[2];
    humidity = humidity << 4;
    humidity |= (data[3] >> 4);
    humidity = (humidity * 100 / 0x100000);

    return humidity;
}

int read_VEML7700(void) {
    // Variable declaration
    uint8_t config_command[3] = {VEML7700_CONFIG, VEML7700_CONFIG_P1, VEML7700_CONFIG_P2};
    uint8_t command[1] = {VEML7700_MEASURE_LIGHT};
    uint8_t data[2];
    int lux;

    // Set VEML7700 to gain of 1 and integration time of 100ms
    ESP_ERROR_CHECK(i2c_master_transmit(VEML7700_dev_handle, config_command, sizeof(config_command), -1));

    // Read light level
    i2c_master_transmit_receive(VEML7700_dev_handle, command, sizeof(command), data, sizeof(data), -1);

    // Combine the two bytes into a 16-bit value
    lux = (data[1] << 8) | data[0];

    // Convert to lux
    lux *= 0.0672;
    if (lux > 1000) {
        float a = 6.0135E-13;
        float b = -9.3924E-9;
        float c = 8.1488E-5;
        float d = 1.0023;
        lux = (int)(pow(lux, 4) * a + pow(lux, 3) * b + pow(lux, 2) * c + lux * d);
    }

    return lux;
}

void EPD_busy(void) {
    // Wait until the EPD is not busy
    while(gpio_get_level(EPD_BUSY_PIN) == 1);
}

void EPD_reset(void) {
    // Set reset pin high
    gpio_set_level(EPD_RST_PIN, GPIO_PIN_SET);
    // Delay 100us
    vTaskDelay(100 / portTICK_PERIOD_MS);
    // Set reset pin low
    gpio_set_level(EPD_RST_PIN, GPIO_PIN_RESET);
    // Delay 2us
    vTaskDelay(2 / portTICK_PERIOD_MS);
    // Set reset pin high
    gpio_set_level(EPD_RST_PIN, GPIO_PIN_SET);
    // Delay 100us
    vTaskDelay(100 / portTICK_PERIOD_MS);
    // Wait for display
    EPD_busy(); 
}

void EPD_turn_on_display(void) {
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

void EPD_set_windows(uint16_t X_start, uint16_t Y_start, uint16_t X_end, uint16_t Y_end) {
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

void EPD_set_cursor(uint16_t X_start, uint16_t Y_start) {
    EPD_send_byte(0x4E, COMMAND); // SET_RAM_X_ADDRESS_COUNTER
    EPD_send_byte(X_start & 0xFF, DATA);

    EPD_send_byte(0x4F, COMMAND); // SET_RAM_Y_ADDRESS_COUNTER
    EPD_send_byte(Y_start & 0xFF, DATA);
    EPD_send_byte((Y_start >> 8) & 0xFF, DATA);
}

void EPD_init(void) {
    // Reset the display
    EPD_reset();
    //EPD_reset();

    // Configure update mode
    EPD_send_byte(EPD_CMD_CONF_UPDATE_MODE_1, COMMAND);
    EPD_send_byte(0x40, DATA);
	
    // Set data entry mode to x address
    EPD_send_byte(EPD_CMD_SET_DATA_ENTRY_MODE, COMMAND);
    EPD_send_byte(0x03, DATA);
		
    // Set the window to the size of the screen
	EPD_set_windows(0, 0, EPD_4IN2_V2_WIDTH-1, EPD_4IN2_V2_HEIGHT-1);

    // Set the cursor at the origin (Top Left)
    EPD_set_cursor(0, 0);

    EPD_busy();
}

void EPD_clear(void) {
    // Calculate width and height
    uint16_t Width = EPD_4IN2_V2_WIDTH / 8; // Divide by 8 since bytes are being sent
    uint16_t Height = EPD_4IN2_V2_HEIGHT;

    // Write to screen in BW
    EPD_send_byte(EPD_CMD_WRITE_BW, COMMAND);
    // Set each pixel to 1
    for (int i = 0; i < Height*Width; i++) {
        EPD_send_byte(0xFF, DATA);
    }

    // Turn on the display
    EPD_turn_on_display();
}

void EPD_clear_image(unsigned char* image) {
    // Calculate width and height
    uint16_t Width = EPD_4IN2_V2_WIDTH / 8; // Divide by 8 since bytes are being sent
    uint16_t Height = EPD_4IN2_V2_HEIGHT;

    // Set each pixel to 1
    for (int i = 0; i < Height*Width; i++) {
        image[i] = 0xFF;
    }
}

void EPD_display_image(unsigned char* image) {
    // Calculate width and height
    uint16_t Width = EPD_4IN2_V2_WIDTH / 8; // Divide by 8 since bytes are being sent
    uint16_t Height = EPD_4IN2_V2_HEIGHT;

    // Write to screen in BW
    EPD_send_byte(EPD_CMD_WRITE_BW, COMMAND);
    // Set each pixel to 1 (black)
    for (int i = 0; i < Height*Width; i++) {
        EPD_send_byte(image[i], DATA);
    }

    // Turn on the display
    EPD_turn_on_display();
}