#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	//check the number of factors: program,original file,copy file
	if(argc !=3){
		fprintf(stderr, "Usage: %s <original file> <copy file> \n", argv[0]);
		return 1;
	}

	//open original file (read mode, binary mode)
	FILE *originalfile = fopen(argv[1], "rb");
	if(originalfile == NULL){
		perror("fail to open original file");
		return 1;
	}

	//Open copy file (writing mode, binary mode; truncate if the file exits)
	FILE *copyfile = fopen(argv[2], "wb");
	if(copyfile ==NULL){
		perror("fail to open copy file");
		fclose(originalfile);
	}

	//10byte read & write
	char buffer[10];
	size_t bytesRead;

	while((bytesRead = fread(buffer, 1, sizeof(buffer), originalfile))>0){
		//write as many bytes read to a copy file
		if(fwrite(buffer, 1, bytesRead, copyfile) != bytesRead) {
			perror("fail to write copy file");
			fclose(originalfile);
			fclose(copyfile);
			return 1;
		}
	}

	// check errors while reading
	if (ferror(originalfile)){
		perror("fail to read original file");
	}

	fclose(originalfile);
	fclose(copyfile);
	return 0;
}
