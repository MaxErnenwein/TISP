#include "../includes/main.h"

extern RTC_DATA_ATTR uint32_t settings;
i2c_master_dev_handle_t MCP9808_dev_handle;
i2c_master_dev_handle_t AHT20_dev_handle;
i2c_master_dev_handle_t VEML7700_dev_handle;
i2c_master_dev_handle_t C4001_dev_handle;
adc_continuous_handle_t SPW2430_dev_handle;

float read_MCP9808(void) {
    // Variable decleration
    float Temp;
    uint8_t data[2];
    uint8_t command = MCP9808_MEASURE_TEMPERATURE;

    // Request and read the temperature data
    ESP_ERROR_CHECK(i2c_master_transmit_receive(MCP9808_dev_handle, &command, sizeof(command), data, sizeof(data), -1));

    // Chech for malfunctioning sensor
    if (((data[0] & 0xFF) == 0xFF) && ((data[1] & 0xFF) == 0xFF)) {
        settings |= (1 << SETTING_MCP9808_STATUS);
        return 0;
    } else {
        settings &= ~(1 << SETTING_MCP9808_STATUS);
    }

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
    int humidity = 0;
    uint8_t command[3] = {AHT20_MEADURE_HUMIDITY, AHT20_MEADURE_HUMIDITY_P1, AHT20_MEADURE_HUMIDITY_P2};
    uint8_t data[7];
    int error = 1;
    
    // Request and read the humidity data
    ESP_ERROR_CHECK(i2c_master_transmit(AHT20_dev_handle, command, sizeof(command), -1));
    ESP_ERROR_CHECK(i2c_master_receive(AHT20_dev_handle, data, sizeof(data), -1));

    // Chech for malfunctioning sensor
    for (int i = 0; i < 7; i++) {
        if ((data[i] & 0xFF) != 0xFF) {
            error = 0;
        }
    }
    if (error) {
        settings |= (1 << SETTING_AHT20_STATUS);
        return 0;
    } else {
        settings &= ~(1 << SETTING_AHT20_STATUS);
    }
        
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
    uint8_t command[1] = {VEML7700_MEASURE_LIGHT};
    uint8_t data[2];
    int lux;

    // Read light level
    ESP_ERROR_CHECK(i2c_master_transmit_receive(VEML7700_dev_handle, command, sizeof(command), data, sizeof(data), -1));

    // Chech for malfunctioning sensor
    if (((data[0] & 0xFF) == 0xFF) && ((data[1] & 0xFF) == 0xFF)) {
        printf("VEML7700 Error\n");
        settings |= (1 << SETTING_VEML7700_STATUS);
        return 0;
    } else {
        settings &= ~(1 << SETTING_VEML7700_STATUS);
    }

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

int read_SPW2430(void) {
    ESP_ERROR_CHECK(adc_continuous_start(SPW2430_dev_handle));

    usleep(5000);

    uint8_t buf[400];
    uint32_t ret_num = 0;
    int max = INT_MIN;
    int min = INT_MAX;

    // Read the actual data
    ESP_ERROR_CHECK(adc_continuous_read(SPW2430_dev_handle, buf, 400, &ret_num, -1));

    int data;
    for (int i = 0; i < 100; i++) {
        data = ((buf[i*4 + 1] & 0x0F) << 8) | buf[i*4];
        if (data > max) {
            max = data;
        }
        if (data < min) {
            min = data;
        }
    }

    int diff = max - min;

    ESP_ERROR_CHECK(adc_continuous_stop(SPW2430_dev_handle));

    return diff;
}

int read_C4001(void) {
    uint8_t data;
    int presence = 0;
    uint8_t command = C4001_GET_PRESENCE;

    // Measure presence
    for(int i = 0; i < 10; i++) {
        ESP_ERROR_CHECK(i2c_master_transmit_receive(C4001_dev_handle, &command, sizeof(command), &data, sizeof(data), -1));
        if (data == 1) {
            presence = 1;
        }
        // Short delay between measurements
        usleep(10000);
    }

    if (data > 1) {
        settings |= (1 << SETTING_C4001_STATUS);
        return 0;
    } else {
        settings &= ~(1 << SETTING_C4001_STATUS);
    }

    return presence;
}

