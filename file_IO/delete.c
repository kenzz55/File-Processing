#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFSIZE 4096

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <offset> <bytes> <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Convert offset and number of bytes to delete from string to long type
    long offset = atol(argv[1]);
    long delBytes = atol(argv[2]);

    if (offset < 0 || delBytes < 0) {
        fprintf(stderr, "Offset과 bytes는 음수가 될 수 없습니다.\n");
        exit(EXIT_FAILURE);
    }

    // if bytes = 0 -> program exit
    if (delBytes == 0) {
        return EXIT_SUCCESS;
    }

    // open the file R/W
    FILE* fp = fopen(argv[3], "r+b");
    if (!fp) {
        perror("파일 열기 실패");
        exit(EXIT_FAILURE);
    }

    // check the file size
    if (fseek(fp, 0, SEEK_END) != 0) {
        perror("fseek 실패");
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    long filesize = ftell(fp);
    if (filesize < 0) {
        perror("ftell 실패");
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    // If the offset is greater than the file size, exit
    if (offset >= filesize) {
        fclose(fp);
        return EXIT_SUCCESS;
    }

    // if delete data size > filesize -> data adjust
    if (offset + delBytes > filesize) {
        delBytes = filesize - offset;
    }

    long newSize = filesize - delBytes;
    char buffer[BUFSIZE];

    // Set the read and write positions
    long readPos = offset + delBytes;
    long writePos = offset;

    while (readPos < filesize) {
        // move to read position
        if (fseek(fp, readPos, SEEK_SET) != 0) {
            perror("읽기 위치 이동 실패");
            fclose(fp);
            exit(EXIT_FAILURE);
        }
        size_t bytesToRead = (filesize - readPos) > BUFSIZE ? BUFSIZE : (filesize - readPos);
        size_t bytesRead = fread(buffer, 1, bytesToRead, fp);
        if (bytesRead == 0) {
            break;
        }
        // Record data from the buffer after moving to the write position
        if (fseek(fp, writePos, SEEK_SET) != 0) {
            perror("쓰기 위치 이동 실패");
            fclose(fp);
            exit(EXIT_FAILURE);
        }
        size_t bytesWritten = fwrite(buffer, 1, bytesRead, fp);
        if (bytesWritten != bytesRead) {
            perror("쓰기 실패");
            fclose(fp);
            exit(EXIT_FAILURE);
        }
        readPos += bytesRead;
        writePos += bytesRead;
    }

    // truncation
    int fd = fileno(fp);
    if (ftruncate(fd, newSize) != 0) {
        perror("파일 크기 조정 실패");
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    fclose(fp);
    return EXIT_SUCCESS;
}
