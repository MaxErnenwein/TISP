#ifndef SD_H
#define SD_H

// Includes
#include "../includes/main.h"

// Defines
// Data file name
#define FILE_LOCATION "/sdcard/TISPdata.bin"

// Function declerations
void SD_write_file(char* file_path, struct sensor_readings data);
void SD_store_sensor_data(int sound);
struct sensor_readings SD_read_file(char* file_path, int index);

#endif // SD_H