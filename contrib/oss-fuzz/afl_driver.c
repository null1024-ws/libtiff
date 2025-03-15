#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Forward declaration of the external function
extern int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size);

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    FILE *input_file = fopen(argv[1], "rb");
    if (!input_file) {
        fprintf(stderr, "Failed to open input file: %s\n", argv[1]);
        return 1;
    }

    // Seek to the end of the file to determine its size
    fseek(input_file, 0, SEEK_END);
    long file_size = ftell(input_file);
    fseek(input_file, 0, SEEK_SET);

    if (file_size < 0) {
        fprintf(stderr, "Failed to determine file size.\n");
        fclose(input_file);
        return 1;
    }

    // Allocate memory for the file contents
    uint8_t *data = (uint8_t *)malloc(file_size);
    if (!data) {
        fprintf(stderr, "Failed to allocate memory.\n");
        fclose(input_file);
        return 1;
    }

    // Read the file into memory
    size_t read_size = fread(data, 1, file_size, input_file);
    fclose(input_file);

    if (read_size != file_size) {
        fprintf(stderr, "Failed to read the entire file.\n");
        free(data);
        return 1;
    }

    // Call the LLVMFuzzerTestOneInput function
    LLVMFuzzerTestOneInput(data, file_size);

    free(data);
    return 0;
}
