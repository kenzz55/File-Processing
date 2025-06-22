#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]){
	if (argc != 4){
		fprintf(stderr, "Useage : %s, <filename1> <filename2> <filename3> \n", argv[0]);
		exit(EXIT_FAILURE);
	}
	const char *file1 = argv[1];
	const char *file2 = argv[2];
	const char *file3 = argv[3];

	//read onyl file2 and file3
	int fd2 = open(file2, O_RDONLY);
	if(fd2<0){
		perror("file2 open error");
		exit(EXIT_FAILURE);
	}
	int fd3 = open(file3, O_RDONLY);
	if(fd3<0){
		perror("file3 open error");
		close(fd2);
		exit(EXIT_FAILURE);
	}
	// open file1 in write only, create if not, delete trunk mode
	int fd1 = open(file1, O_WRONLY | O_CREAT | O_TRUNC, 0664);
	if(fd1 <0){
		perror("file1 open error");
		close(fd2);
		close(fd3);
		exit(EXIT_FAILURE);
	}

	char buffer[BUFFER_SIZE];
	ssize_t bytesRead, bytesWritten;

	//read file2 data and write it to file1
	while ((bytesRead = read(fd2,buffer, BUFFER_SIZE))>0){
		bytesWritten = write(fd1, buffer, bytesRead);
		if(bytesWritten != bytesRead){
			perror("file1 write error");
			close(fd1);
			close(fd2);
			close(fd3);
			exit(EXIT_FAILURE);
		}
	}
	if (bytesRead < 0 ) {
		perror("file2 read error");
		close(fd1);
		close(fd2);
		close(fd3);
		exit(EXIT_FAILURE);
	}

	//read file3 data and write it to file1
	while ((bytesRead = read(fd3,buffer, BUFFER_SIZE))>0){
		bytesWritten = write(fd1,buffer,bytesRead);
		if(bytesWritten != bytesRead){
			perror("file1 write error");
			close(fd1);
			close(fd2);
			close(fd3);
			exit(EXIT_FAILURE);
		}
	}
	if(bytesRead < 0 ){
		perror("file3 read error");
		close(fd1);
		close(fd2);
		close(fd3);
		exit(EXIT_FAILURE);
	}

	close(fd1);
	close(fd2);
	close(fd3);


	return 0;
}

