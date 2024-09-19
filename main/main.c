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

void EPD_draw_pixel(uint16_t x, uint16_t y, unsigned char* image) {
    // Check to see if pixel is on screen
    if (x > EPD_4IN2_V2_WIDTH || y > EPD_4IN2_V2_HEIGHT) {
        printf("pixel out of screen range\n");
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
    int main_direction; // Larger change start position
    int secondary_direction; // Smaller change start position
    int direction = abs((int)X_diff) >= abs((int)Y_diff); // Determine what direction is the main (larger) one

    // Set variables depending on the main direction
    if (direction) {
        greater = X_diff;
        lesser = Y_diff;
        ratio = Y_diff / X_diff;
        main_direction = X_start;
        secondary_direction = Y_start;
    } else {
        greater = Y_diff;
        lesser = X_diff;
        ratio = X_diff / Y_diff;
        main_direction = Y_start;
        secondary_direction = X_start;
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
            EPD_draw_pixel(main_direction + (i * main_sign), secondary_direction + (int)(i * ratio * secondary_sign), image);
        } else {
            EPD_draw_pixel(secondary_direction + (int)(i * ratio * secondary_sign), main_direction + (i * main_sign), image);
        }
    }
}

void initial_startup(void) {
    printf("Initial Startup\n");

    // Configure peripherals
    peripherals_init();

    // Display startup image
    EPD_init();
    EPD_display_image(butterfly_image);
    EPD_deep_sleep();
}

void GPIO_wakeup_startup(void) {
    printf("GPIO Wakeup\n");

    // Configure peripherals
    peripherals_init();

    // Draw sensor data to image
    //EPD_draw_sensor_data();

    EPD_draw_line(200, 150, 0, 50, current_data_image);
    EPD_draw_line(50, 0, 200, 150, current_data_image);
    EPD_draw_line(200, 150, 350, 0, current_data_image);
    EPD_draw_line(400, 50, 200, 150, current_data_image);
    EPD_draw_line(200, 150, 400, 250, current_data_image);
    EPD_draw_line(350, 300, 200, 150, current_data_image);
    EPD_draw_line(200, 150, 50, 300, current_data_image);
    EPD_draw_line(0, 250, 200, 150, current_data_image);

    // Display current sensor data
    EPD_init();
    EPD_display_image(current_data_image);
    EPD_deep_sleep();
}

void timer_wakeup_startup(void) {
    printf("Timer Wakeup\n");

    // Configure peripherals
    peripherals_init();
}

void peripherals_init(void) {
    // Configure GPIO
    GPIO_init();

    // Initialize i2c
    I2C_init();

    // Initialize SPI
    SPI_init();
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

void EPD_draw_sensor_data(void) {
    // Read temp from MCP9808
    float Temp;
    Temp = read_MCP9808();

    // Read temp from MCP9808
    float Temp_2;
    Temp_2 = read_MCP9808_2();

    // Declare strings
    char temp_string[20];
    char temp_string_2[22];

    // Convert temperature value to string
    snprintf(temp_string, sizeof(temp_string), "Temperature: %.2fC", Temp);
    snprintf(temp_string_2, sizeof(temp_string_2), "Temperature 2: %.2fC", Temp_2);

    // Draw temperature values to display
    EPD_draw_string(0, 0, temp_string, sizeof(temp_string), FONT12_HEIGHT, current_data_image);
    EPD_draw_string(0, 14, temp_string_2, sizeof(temp_string_2), FONT12_HEIGHT, current_data_image);

    // Print sensor values
    printf("Temperature: %.4f °C\n", Temp);
    printf("Temperature_2: %.4f °C\n", Temp_2);
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
    // Set each pixel to 1 (black)
    for (int i = 0; i < Height*Width; i++) {
        EPD_send_byte(0xFF, DATA);
    }

    // Turn on the display
    EPD_turn_on_display();
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