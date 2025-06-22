#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
  
    // Check the number of factors: a.out <offset> <number of bytes> <file name>
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <offset> <byte_count> <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Converting command line factors to integers (offset and bytes)
    long offset = atoi(argv[1]);
    int countbytes = atoi(argv[2]);
    const char* filename = argv[3];

    //if bytes is 0
    if (countbytes == 0) {
        return 0;
    }

    // open file
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    //check the file size
    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size == -1) {
        perror("Error seeking file");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Shutdown if offset exceeds file size
    if (offset >= file_size) {
        close(fd);
        return 0;
    }

    if (offset + countbytes > file_size) {
        countbytes = file_size - offset;
    }

    //move to offset
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("Error seeking to offset");
        close(fd);
        exit(EXIT_FAILURE);
    }

    //read to data
    char* buffer = (char*)malloc(countbytes + 1);
    if (buffer == NULL) {
        perror("Error allocating memory");
        close(fd);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read = read(fd, buffer, countbytes);
    if (bytes_read == -1) {
        perror("Error reading file");
        free(buffer);
        close(fd);
        exit(EXIT_FAILURE);
    }


    buffer[bytes_read] = '\0'; 
    printf("%s", buffer);


    free(buffer);
    close(fd);

    return 0;
}
