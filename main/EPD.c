#include "../includes/EPD.h"
#include "../includes/fonts.h"
#include "../includes/main.h"
#include "../includes/Sensors.h"

spi_device_handle_t EPD_dev_handle;
extern RTC_DATA_ATTR uint32_t settings;

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

    usleep(10000);
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
    EPD_send_byte(EPD_CMD_SET_RAM_X_ADDRESS_COUNTER, COMMAND); // SET_RAM_X_ADDRESS_COUNTER
    EPD_send_byte(X_start & 0xFF, DATA);

    EPD_send_byte(EPD_CMD_SET_RAM_Y_ADDRESS_COUNTER, COMMAND); // SET_RAM_Y_ADDRESS_COUNTER
    EPD_send_byte(Y_start & 0xFF, DATA);
    EPD_send_byte((Y_start >> 8) & 0xFF, DATA);
}

void EPD_turn_on_display(void) {
    // Set update mode
    EPD_send_byte(EPD_CMD_CONF_UPDATE_MODE_2, COMMAND);
	EPD_send_byte(0xF7, DATA);
    // Update display
    EPD_send_byte(EPD_CMD_UPDATE_DISPLAY, COMMAND);
    // Short delay
    usleep(10000);
}

void EPD_reset(void) {
    // Set reset pin high
    gpio_set_level(EPD_RST_PIN, GPIO_PIN_SET);
    // Delay 100ms
    usleep(100000);
    // Set reset pin low
    gpio_set_level(EPD_RST_PIN, GPIO_PIN_RESET);
    // Delay 2ms
    usleep(2000);
    // Set reset pin high
    gpio_set_level(EPD_RST_PIN, GPIO_PIN_SET);
    // Delay 100ms
    usleep(100000);
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
        case 24: 
            font_width = FONT24_WIDTH;
            break;
        default: 
            font_width = 0;
            break;
    }
    uint32_t row_holder;

    // Loop through each byte in the font
    for (int i = 0; i < font_size; i++) {
        // Loop through each pixel in the width of the font
        for (int j = 0; j < font_width; j++) {
            // Determine what font table to use
            if(font_size == 8) {
                // Check if pixel should be drawn
                if((((Font8_Table[font_character_index + i]) >> (7 - j)) & 0x01) == 0x01) {
                    EPD_draw_pixel(x + j, y + i, image);
                }
            } else if (font_size == 12) {
                // Check if pixel should be drawn
                if((((Font12_Table[font_character_index + i]) >> (7 - j)) & 0x01) == 0x01) {
                    EPD_draw_pixel(x + j, y + i, image);
                }
            } else if (font_size == 24) {
                // Check if pixel should be drawn
                row_holder = (Font24_Table[font_character_index + (i * 3)] << 16) + (Font24_Table[font_character_index + (i * 3) + 1] << 8) + (Font24_Table[font_character_index + (i * 3) + 2]);
                if(((row_holder >> (23 - j)) & 0x01) == 0x01) {
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
        case 24: 
            font_width = FONT24_WIDTH;
            break;
        default: 
            font_width = 0;
            break;
    }

    // Print each character in the passed string
    for (int i = 0; i <= string_size - 2; i++) {
        // Get the next character
        character = string[i];

        // Check for end of string
        if (character == '~') {
            break;
        }

        // Calculate index into font table
        if (font_size <= 12) {
            index = (((int)character) - 32) * font_size;
        } else {
            index = (((int)character) - 32) * font_size * 3;
        }
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

void EPD_draw_sensor_data(unsigned char* image, int sound) {
    // Read temp from MCP9808
    float temp;
    temp = read_MCP9808();

    // Read humidity from AHT20
    int humidity;
    humidity = read_AHT20();

    // Read light level from VEML7700
    int lux;
    lux = read_VEML7700();

    // Read sound from SPW2430
    //int sound;
    //sound = read_SPW2430();

    // Read presence from C4001
    int presence;
    presence = read_C4001();

    // Declare strings
    char temp_string[10];
    char humidity_string[5];
    char lux_string[11];  
    char sound_string[9];
    char presence_string[5];

    // Convert values to strings
    snprintf(temp_string, sizeof(temp_string), "%.2f C~", temp);
    snprintf(humidity_string, sizeof(humidity_string), "%d%%~", humidity);
    snprintf(lux_string, sizeof(lux_string), "%d Lux~", lux);
    snprintf(sound_string, sizeof(sound_string), "%dpp~", sound);
    if(presence) {
        snprintf(presence_string, sizeof(presence_string), "Yes~");
    } else {
        snprintf(presence_string, sizeof(presence_string), "No~");
    }

    // Declare strings
    char TISP_string_1[] = "Current Measurements";
    char TISP_string_2[] = "TISP Systems Made By:";
    char TISP_string_3[] = "Max Ernenwein &";
    char TISP_string_4[] = "Noah Lambert";

    // Get sensor status values
    uint8_t sensor_status = (settings >> SETTING_MCP9808_STATUS) & 0x1F;

    // Draw values to display
    // Temperature
    if ((sensor_status & 0x01) == 0x01) {
        EPD_draw_string(40, 40, "Error", 6, FONT24_HEIGHT, image);
    } else {
        EPD_draw_string(40, 40, temp_string, sizeof(temp_string), FONT24_HEIGHT, image);
    }
    // Humididy
    if ((sensor_status & 0x02) == 0x02) {
        EPD_draw_string(215, 40, "Error", 6, FONT24_HEIGHT, image);
    } else {
        EPD_draw_string(215, 40, humidity_string, sizeof(humidity_string), FONT24_HEIGHT, image);
    }
    // Light
    if ((sensor_status & 0x04) == 0x04) {
        EPD_draw_string(50, 140, "Error", 6, FONT24_HEIGHT, image);
    } else {
        EPD_draw_string(50, 140, lux_string, sizeof(lux_string), FONT24_HEIGHT, image);
    }
    // Presence
    if ((sensor_status & 0x08) == 0x08) {
        EPD_draw_string(40, 240, "Error", 6, FONT24_HEIGHT, image);
    } else {
        EPD_draw_string(40, 240, presence_string, sizeof(presence_string), FONT24_HEIGHT, image);
    }
    // Sound
    if ((sensor_status & 0x10) == 0x10) {
        EPD_draw_string(215, 140, "Error", 6, FONT24_HEIGHT, image);
    } else {
        EPD_draw_string(215, 140, sound_string, sizeof(sound_string), FONT24_HEIGHT, image);
    }
    // Test
    EPD_draw_string(210, 210, TISP_string_1, sizeof(TISP_string_1), FONT12_HEIGHT, image);
    EPD_draw_string(210, 230, TISP_string_2, sizeof(TISP_string_2), FONT12_HEIGHT, image);
    EPD_draw_string(210, 250, TISP_string_3, sizeof(TISP_string_3), FONT12_HEIGHT, image);
    EPD_draw_string(210, 270, TISP_string_4, sizeof(TISP_string_4), FONT12_HEIGHT, image);

    // Draw the image
    EPD_display_image(image);
}

void EPD_display_image(unsigned char* image) {
    // Calculate width and height
    uint16_t Width = EPD_4IN2_V2_WIDTH / 8; // Divide by 8 since bytes are being sent
    uint16_t Height = EPD_4IN2_V2_HEIGHT;

    // Write to screen in BW
    EPD_send_byte(EPD_CMD_WRITE_BW, COMMAND);
    for (int i = 0; i < Height*Width; i++) {
        EPD_send_byte(image[i], DATA);
    }

    // Turn on the display
    EPD_turn_on_display();
}

int EPD_draw_graph(int variable, int delta_time, char* file_path, unsigned char* image) {
    // Open the data in SD card
    FILE* file = fopen(file_path, "rb");

    // Check for file open error
    if (file == NULL) {
        printf("File failed to open\n");
        return 1;
    }

    // Declare vairables
    struct sensor_readings data = {0};
    struct sensor_readings empty_struct = {0};
    int graph_data_int[NUM_DATA_POINTS];
    int largest_int = INT_MIN;
    int smallest_int = INT_MAX;
    float graph_data_flt[NUM_DATA_POINTS];
    float largest_flt = INT_MIN;
    float smallest_flt = INT_MAX;
    int points_cnt = NUM_DATA_POINTS;
    double sum = 0;
    float average;
    int bad_data_points_cnt = 0;
    int ret;

    // Go to last data measurement
    fseek(file, -1 * (sizeof(struct sensor_readings)), SEEK_END);

    // Read data needed from file
    for (int i = 0; i < points_cnt; i++) {

        // Read data from file
        int read = fread(&data, sizeof(struct sensor_readings), 1, file);
        if (read != 1) {
            // If the data point wasn't read due to reaching beginning of file, remove point from graph
            printf("Failed to read file data point %d\n", i);
            data = empty_struct;
            points_cnt--;
        }

        // Put desired data in array
        switch (variable) {
            case 0: 
                if (((data.status >> 0) & 0x01) == 0x01) {
                    graph_data_flt[i] = INT_MIN;
                } else {
                    graph_data_flt[i] = data.temperature;
                }
                break;
            case 1: 
                if (((data.status >> 1) & 0x01) == 0x01) {
                    graph_data_int[i] = INT_MIN;
                } else {
                    graph_data_int[i] = data.humidity;
                }
                break;
            case 2: 
                if (((data.status >> 2) & 0x01) == 0x01) {
                    graph_data_int[i] = INT_MIN;
                } else {
                    graph_data_int[i] = data.light;
                }
                break;
            case 3:
                graph_data_int[i] = data.sound;
                break;
            case 4: 
                if (((data.status >> 4) & 0x01) == 0x01) {
                    graph_data_int[i] = INT_MIN;
                } else {
                    graph_data_int[i] = data.presence;
                }
                break;
            default: 
                return 0;
                break;
        }

        // Find the largest value, smallest value, and average in the data
        if (variable < 1 && graph_data_flt[i] != INT_MIN) {
            if (graph_data_flt[i] > largest_flt) {
                largest_flt = graph_data_flt[i];
            } else if (graph_data_flt[i] < smallest_flt) {
                smallest_flt = graph_data_flt[i];
            }
            sum += graph_data_flt[i];
        } else if (graph_data_int[i] != INT_MIN) {
            if (graph_data_int[i] > largest_int) {
                largest_int = graph_data_int[i];
            } else if (graph_data_int[i] < smallest_int) {
                smallest_int = graph_data_int[i];
            }
            sum += graph_data_int[i];
        } else {
            bad_data_points_cnt += 1;
        }

        // Go to previous data point in file
        ret = fseek(file, -1 * (sizeof(struct sensor_readings) + (delta_time * sizeof(struct sensor_readings))), SEEK_CUR);
        if (ret != 0) {
            return 1;
            printf("Seek failure\n");
        }
    }

    // Calucate average
    average = sum / (NUM_DATA_POINTS - bad_data_points_cnt);

    // Find the change of one pixel in the y direction
    float delta_y_pixel;
    if (variable < 1) {
        delta_y_pixel = ((float)(largest_flt - smallest_flt))/GRAPH_Y_PIXELS;
    } else {
        delta_y_pixel = ((float)(largest_int - smallest_int))/GRAPH_Y_PIXELS;
    }

    // Draw the lines in the graph
    int X_start, X_end, Y_start, Y_end;
    for (int i = 0; i < points_cnt - 1; i++) {
        // Calculate x direction
        //X_start = GRAPH_X_OFFSET + (i * GRAPH_X_DELTA); 
        //X_end = GRAPH_X_OFFSET + GRAPH_X_DELTA + (i * GRAPH_X_DELTA);
        // New graph direction to make Sheaff happy
        X_start = GRAPH_X_LIMIT - (i * GRAPH_X_DELTA);
        X_end = GRAPH_X_LIMIT - (GRAPH_X_DELTA + (i * GRAPH_X_DELTA));
        // Depending on what type the data is, calculate the y direction
        if (variable < 1) {
            // Don't graph bad sensor data
            if (graph_data_flt[i] != INT_MIN && graph_data_flt[i+1] != INT_MIN) {
                Y_start = (int)(((float)(largest_flt - graph_data_flt[i]))/delta_y_pixel) + GRAPH_Y_OFFSET;
                Y_end = (int)(((float)(largest_flt - graph_data_flt[i + 1]))/delta_y_pixel) + GRAPH_Y_OFFSET;
                // Draw the graph line
                EPD_draw_line(X_start, Y_start, X_end, Y_end, image);
            }
        } else {
            if (graph_data_int[i] != INT_MIN && graph_data_int[i+1] != INT_MIN) {
                Y_start = (int)(((float)(largest_int - graph_data_int[i]))/delta_y_pixel) + GRAPH_Y_OFFSET;
                Y_end = (int)(((float)(largest_int - graph_data_int[i + 1]))/delta_y_pixel) + GRAPH_Y_OFFSET;
                // Draw the graph line
                EPD_draw_line(X_start, Y_start, X_end, Y_end, image);
            }
        }
    }

    // Draw Axes
    // Y-Axis
    EPD_draw_line(GRAPH_X_OFFSET, GRAPH_Y_LIMIT, GRAPH_X_OFFSET, GRAPH_Y_OFFSET, image);
    // X-Axis
    EPD_draw_line(GRAPH_X_OFFSET, GRAPH_Y_LIMIT, 360, GRAPH_Y_LIMIT, image);
    // Axes Labels
    char xvar[5] = "Time";
    EPD_draw_string(365, 275, xvar, sizeof(xvar), FONT12_HEIGHT, image);
    // Y Hatch Marks
    // Declare variables
    float ratio;
    float flt_value;
    int16_t int_value;
    int var_offset = 0;
    char average_string[23];
    switch (variable) {
        case 0:
            // Label Y-Axis
            char temp_string[] = "Temperature (C)";
            EPD_draw_string(0, 0, temp_string, sizeof(temp_string), FONT12_HEIGHT, image);
            // Label hatch marks
            char temp_value[6];
            ratio = (largest_flt - smallest_flt) / GRAPH_Y_PIXELS;
            for (int i = 0; i < 11; i++) {
                // Draw hatch mark
                EPD_draw_line(GRAPH_X_OFFSET - 2, (i * GRAPH_Y_HATCH_DELTA) + GRAPH_Y_OFFSET, GRAPH_X_OFFSET + 2, (i * GRAPH_Y_HATCH_DELTA) + GRAPH_Y_OFFSET, image);
                // Calculate the value for the hatch mark
                flt_value = largest_flt - (((i * GRAPH_Y_HATCH_DELTA) + GRAPH_Y_HATCH_DELTA) * ratio);
                // Draw label for hatch mark
                snprintf(temp_value, sizeof(temp_value), "%.1f~", flt_value);
                EPD_draw_string(0, (i * GRAPH_Y_HATCH_DELTA) + GRAPH_Y_OFFSET, temp_value, sizeof(temp_value) - var_offset, FONT12_HEIGHT, image);
                var_offset = 0;
            }
            // Create average string
            snprintf(average_string, sizeof(average_string), "Average: %.2f C~", average);
            EPD_draw_string(150, 0, average_string, sizeof(average_string), FONT12_HEIGHT, image);
            break;
        case 1:
            // Label Y-Axis
            char humidity_string[9] = "Humidity";
            EPD_draw_string(0, 0, humidity_string, sizeof(humidity_string), FONT12_HEIGHT, image);
            // Label hatch marks
            char humidity_value[5];
            ratio = (float)(largest_int - smallest_int) / GRAPH_Y_PIXELS;
            for (int i = 0; i < GRAPH_Y_NUM_HATCH; i++) {
                // Draw hatch mark
                EPD_draw_line(GRAPH_X_OFFSET - 2, (i * GRAPH_Y_HATCH_DELTA) + GRAPH_Y_OFFSET, GRAPH_X_OFFSET + 2, (i * GRAPH_Y_HATCH_DELTA) + GRAPH_Y_OFFSET, image);
                int_value = largest_int - (int)((float)((i * GRAPH_Y_HATCH_DELTA) + GRAPH_Y_OFFSET) * ratio);
                // Make sure humidity value is between 0 and 99
                if (int_value >= 0 && int_value <= 99) {
                    // Draw label for hatch mark
                    snprintf(humidity_value, sizeof(humidity_value), "%2d%%~", int_value);
                    EPD_draw_string(0, (i * GRAPH_Y_HATCH_DELTA) + GRAPH_Y_OFFSET, humidity_value, sizeof(humidity_value), FONT12_HEIGHT, image);
                }
            }
            // Create average string
            snprintf(average_string, sizeof(average_string), "Average: %.2f%%~", average);
            EPD_draw_string(150, 0, average_string, sizeof(average_string), FONT12_HEIGHT, image);
            break;
        case 2:
            // Label Y-axis
            char light_string[12] = "Light (lux)";
            EPD_draw_string(0, 0, light_string, sizeof(light_string), FONT12_HEIGHT, image);
            // Label hatch marks
            char light_value[5];
            ratio = (float)(largest_int - smallest_int) / GRAPH_Y_PIXELS;
            for (int i = 0; i < GRAPH_Y_NUM_HATCH; i++) {
                // Draw hatch marks
                EPD_draw_line(GRAPH_X_OFFSET - 2, (i * GRAPH_Y_HATCH_DELTA) + GRAPH_Y_OFFSET, GRAPH_X_OFFSET + 2, (i * GRAPH_Y_HATCH_DELTA) + GRAPH_Y_OFFSET, image);
                int_value = largest_int - (int)((float)((i * GRAPH_Y_HATCH_DELTA) + GRAPH_Y_OFFSET) * ratio);
                // Make sure light value is between 0 and 9999
                if (int_value >= 0 && int_value <= 9999) {
                    // Draw label for hitch mark
                    snprintf(light_value, sizeof(light_value), "%4d", int_value);
                    EPD_draw_string(0, (i * GRAPH_Y_HATCH_DELTA) + GRAPH_Y_OFFSET, light_value, sizeof(light_value), FONT12_HEIGHT, image);
                }
            }
            // Create average string
            snprintf(average_string, sizeof(average_string), "Average: %.2f lux~", average);
            EPD_draw_string(150, 0, average_string, sizeof(average_string), FONT12_HEIGHT, image);
            break;
        case 3:
            // Label Y-axis
            char sound_string[12] = "Sound level";
            EPD_draw_string(0, 0, sound_string, sizeof(sound_string), FONT12_HEIGHT, image);
            break;
        case 4: 
            // Label Y-axis
            char presence_string[15] = "Human presence";
            EPD_draw_string(0, 0, presence_string, sizeof(presence_string), FONT12_HEIGHT, image);
            break;
        default:
            return 0;
            break;
    }
    
    // X Hatch Marks
    // Declare variables
    char time_string[6];
    char time_unit;
    float time_value = 0;
    int time_span = NUM_DATA_POINTS * delta_time;
    for (int i = 0; i < GRAPH_X_NUM_HATCH; i++) {
        // Draw hatch mark
        EPD_draw_line((i * GRAPH_X_HATCH_DELTA) + GRAPH_X_OFFSET, GRAPH_Y_LIMIT - 2, (i * GRAPH_X_HATCH_DELTA) + GRAPH_X_OFFSET, GRAPH_Y_LIMIT + 2, image);
        time_value = i * (time_span / 6);
        // Change time unit and value based off of how long ago measurement was taken
        if (time_value > 1440) {
            time_unit = 'd';
            time_value = time_value / 1440;
        } else if (time_value > 60) {
            time_unit = 'h';
            time_value = time_value / 60;
        } else {
            time_unit = 'm';
        }
        if (time_value < 10) {
            var_offset += 1;
        }
        // Make sure value is between 0 and 60
        if (time_value >= 0 && time_value <= 60) {
            // Draw label for hatch mark
            snprintf(time_string, sizeof(time_string) - var_offset, "%.1f%c", time_value, time_unit);
            EPD_draw_string(GRAPH_X_LIMIT - (((i + 1) * GRAPH_X_HATCH_DELTA)), 290, time_string, sizeof(time_string) - var_offset, FONT12_HEIGHT, image);
        }
        var_offset = 0;
    }

    // Close the file
    fclose(file);

    // Display the graph
    EPD_display_image(image);

    return 0;
}

void EPD_deep_sleep(void) {
    // Put the display into deep sleep
    EPD_send_byte(EPD_CMD_ENTER_DEEP_SLEEP, COMMAND);
    EPD_send_byte(0x01, DATA);
    printf("EPD_put to sleep\n");
}





