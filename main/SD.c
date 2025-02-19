#include "../includes/SD.h"
#include "../includes/main.h"

sdmmc_card_t *card;
extern RTC_DATA_ATTR uint32_t settings;

void SD_write_file(char* file_path, struct sensor_readings data) {
    FILE* file;
    int ret;

    // Open the data file
    file = fopen(file_path, "ab");

    // Check for file open error
    if (file == NULL) {
        settings |= (1 << SETTING_SD_STATUS);
    }

    // Write data to SD card
    int written = fwrite(&data, sizeof(data), 1, file);

    // Check if data was written
    if (written != 1) {
        printf("Error writing to file in SD_write_file");
        settings |= (1 << SETTING_SD_STATUS);
    }

    // Close the file
    ret = fclose(file);
    if (ret != 0) {
        printf("Failed to close file\n");
    }
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

void SD_store_sensor_data(int sound) {
    // Get sensor values
    struct sensor_readings data = {
        .temperature = read_MCP9808(),
        .humidity = read_AHT20(),
        .light = read_VEML7700(),
        .sound = sound,
        .presence = read_C4001(),
        .status = settings >> SETTING_MCP9808_STATUS
    };
    
    // Create or write to file
    char *data_file = FILE_LOCATION;
    SD_write_file(data_file, data);
}
