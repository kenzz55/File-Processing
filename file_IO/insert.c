#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>   // for ftruncate()
#include <fcntl.h>    // for fileno()

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <offset> <data> <filename>\n", argv[0]);
        return 1;
    }

    // Parse input arguments: offset, insertion data, filename
    long offset = atol(argv[1]);
    char* data = argv[2];  // The shell removes the double quotes
    int dataLen = strlen(data);

    // Open the file in binary read/write mode
    FILE* fp = fopen(argv[3], "rb+");
    if (!fp) {
        perror("Error opening file");
        return 1;
    }

    // Determine the file size
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);

    // For an empty file 
    if (fileSize == 0) {
        if (offset != 0) {
            fprintf(stderr, "For an empty file, only offset 0 is allowed.\n");
            fclose(fp);
            return 1;
        }
    }
    else {
        // For a non-empty file
        if (offset < 0 || offset > fileSize - 1) {
            fprintf(stderr, "Offset must be between 0 and %ld.\n", fileSize - 1);
            fclose(fp);
            return 1;
        }
    }

    // Determine the insertion position
    long insertionPos = (fileSize == 0) ? 0 : offset + 1;

    // Read the tail part
    long tailSize = fileSize - insertionPos;
    char* tailBuffer = NULL;
    if (tailSize > 0) {
        tailBuffer = malloc(tailSize);
        if (!tailBuffer) {
            perror("Memory allocation error");
            fclose(fp);
            return 1;
        }
        fseek(fp, insertionPos, SEEK_SET);
        if (fread(tailBuffer, 1, tailSize, fp) != (size_t)tailSize) {
            perror("Error reading the tail of the file");
            free(tailBuffer);
            fclose(fp);
            return 1;
        }
    }

    // Move the file pointer to the insertion position and write the insertion data
    fseek(fp, insertionPos, SEEK_SET);
    if (fwrite(data, 1, dataLen, fp) != (size_t)dataLen) {
        perror("Error writing the insertion data");
        if (tailBuffer) free(tailBuffer);
        fclose(fp);
        return 1;
    }

    // Write back the tail data
    if (tailSize > 0) {
        if (fwrite(tailBuffer, 1, tailSize, fp) != (size_t)tailSize) {
            perror("Error writing the tail data");
            free(tailBuffer);
            fclose(fp);
            return 1;
        }
        free(tailBuffer);
    }

    // Flush changes to disk
    fflush(fp);
    // Adjust the file size to be exactly the original size plus the size of the inserted data
    if (ftruncate(fileno(fp), fileSize + dataLen) != 0) {
        perror("Error adjusting file size");
        fclose(fp);
        return 1;
    }

    fclose(fp);
    return 0;
}
