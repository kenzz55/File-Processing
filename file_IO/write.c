#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
    // Check the number of factors
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <offset> <data> <filename>\n", argv[0]);
        return 1;
    }

    // Converting Offset to an integer
    long offset = atol(argv[1]);
    if (offset < 0) {
        fprintf(stderr, "Offset must be a non-negative integer.\n");
        return 1;
    }

    // data
    char* data = argv[2];
    size_t data_write = strlen(data);

    // open the file(read and write)
    FILE* file = fopen(argv[3], "r+");
    if (file == NULL) {
        perror("Failed to open file");
        return 1;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        perror("Failed to seek to end of file");
        fclose(file);
        return 1;
    }
    long file_size = ftell(file);
    if (file_size == -1L) {
        perror("Failed to get file size");
        fclose(file);
        return 1;
    }
    if (offset > file_size) {
        fclose(file);
        return 1;
    }

    // Move the file pointer to the offset position
    if (fseek(file, offset, SEEK_SET) != 0) {
        perror("Failed to seek file");
        fclose(file);
        return 1;
    }

    // write the data
    size_t written = fwrite(data, sizeof(char), data_write, file);
    if (written != data_write) {
        perror("Failed to write data");
        fclose(file);
        return 1;
    }

    // close file
    fclose(file);
    return 0;
}
