#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    // Open raw image file
    const char* raw_filename = "image.raw";
    FILE* raw_file = fopen(raw_filename, "rb");

    // Check for file open failure
    if (raw_file == NULL) {
        printf("Failed to open raw file\n");
        return 1;
    }

    // Open converted image file
    const char* hex_filename = "image_hex.txt";
    FILE* hex_file = fopen(hex_filename, "w");

    // Check for file open failure
    if (hex_file == NULL) {
        printf("Failed to open hex file\n");
        fclose(raw_file);
        return 1;
    }

    // Variables to hold byte information
    uint8_t input_bits[8] = {0};
    uint8_t output_byte = 0;

    // Write opening brace
    fprintf(hex_file, "{");

    // Read each byte in image
    for (int i = 0; i < 15000; i++) {
        // Read byte
        if (fread(&input_bits, sizeof(input_bits), 1, raw_file) != 1) {
            // EOF or error
            break;
        }

        for (int j = 0; j < 8; j++) {
            output_byte |= input_bits[j] << (7 - j);
        }
        
        // Write hex value
        fprintf(hex_file, "0x%02X", output_byte);
        
        // Write comma if not last byte
        if (i < 15000 - 1 && !feof(raw_file)) {
            fprintf(hex_file, ", ");
        }

        // Write newline every 50 bytes
        if ((i % 50 == 49) && (i > 0)) {
            fprintf(hex_file, "\n");
        }

        output_byte = 0;
    }

    // Write closing brace
    fprintf(hex_file, "}");

    fclose(raw_file);
    fclose(hex_file);

    printf("Conversion completed successfully.\n");
    return 0;
}