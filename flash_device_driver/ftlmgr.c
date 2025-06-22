#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "flash.h"


FILE *flashmemoryfp;	// fdevicedriver.c에서 사용
extern int fdd_read(int ppn, char *pagebuf);
extern int fdd_write(int ppn, char *pagebuf);
extern int fdd_erase(int ppn);


int main(int argc, char *argv[])
{	
    if (argc < 2) {
        // 명령이 없는 경우 종료 (출력 요구 없음)
        return 0;
    }

    char command = argv[1][0];

    
    if (command == 'c') {
        if (argc != 4) return 0;  // 인자 부족 시 그냥 종료

        char* flashfile = argv[2];
        int numBlocks = atoi(argv[3]);

        // 파일 생성 ("wb+") → 블록 단위 erase로 0xFF 초기화
        flashmemoryfp = fopen(flashfile, "wb+");
        if (flashmemoryfp == NULL) {
            return 0;
        }
        // 모든 블록 0xFF 초기화
        for (int i = 0; i < numBlocks; i++) {
            fdd_erase(i);  // 각 블록 erase
        }
        fclose(flashmemoryfp);
    }


    else if (command == 'w') {
        if (argc != 6) return 0;

        char* flashfile = argv[2];
        int ppn = atoi(argv[3]);
        char* sectordata = argv[4];
        char* spareStr = argv[5];
        int spareVal = atoi(spareStr);  // 스페어 영역에 기록할 정수

        // "rb+"로 파일 열기
        flashmemoryfp = fopen(flashfile, "rb+");
        if (flashmemoryfp == NULL) {
            return 0;
        }
	char tempPage[PAGE_SIZE];
	if(fdd_read(ppn,tempPage) != 1){
		fclose(flashmemoryfp);
		return 0;
	}
	int isEmpty = 1;
	for(int i = 0; i < SECTOR_SIZE; i++){
		if((unsigned char)tempPage[i] != 0xFF){
			isEmpty = 0;
			break;
		}
	}

	if(!isEmpty){
		fclose(flashmemoryfp);
		return 0;
	}

        // 페이지 버퍼 (528B) 전부 0xFF로 채움
        char pagebuf[PAGE_SIZE];
        memset(pagebuf, 0xFF, PAGE_SIZE);

        // 섹터영역(512B)에 sectordata 복사
        int len = strlen(sectordata);
        if (len > SECTOR_SIZE) len = SECTOR_SIZE;
        memcpy(pagebuf, sectordata, len);

        // 스페어영역(16B) 중 앞 4바이트에 spareVal(정수) 저장 (binary)
        memcpy(pagebuf + SECTOR_SIZE, &spareVal, sizeof(int));

        // fdd_write()로 ppn에 쓰기
        fdd_write(ppn, pagebuf);

        fclose(flashmemoryfp);
    }


    else if (command == 'r') {
        if (argc != 4) return 0;

        char* flashfile = argv[2];
        int ppn = atoi(argv[3]);

        flashmemoryfp = fopen(flashfile, "rb");
        if (flashmemoryfp == NULL) {
            return 0;
        }

        char pagebuf[PAGE_SIZE];
        if (fdd_read(ppn, pagebuf) != 1) {
            fclose(flashmemoryfp);
            return 0;
        }

        // 섹터 영역(512B)에서 첫 번째 0xFF까지를 유효 데이터로 간주
        int i;
        for (i = 0; i < SECTOR_SIZE; i++) {
            if ((unsigned char)pagebuf[i] == 0xFF) {
                break;
            }
        }
        // i==0 이면 섹터 데이터가 없음 → 출력 X
        if (i > 0) {
            // 섹터 데이터를 문자열로 추출
            char sectorData[i + 1];
            memcpy(sectorData, pagebuf, i);
            sectorData[i] = '\0';

            // 스페어 영역에서 정수 추출 (앞 4바이트)
            int spareVal;
            memcpy(&spareVal, pagebuf + SECTOR_SIZE, sizeof(int));

            // 화면에 "섹터데이터 스페어정수" 형태로 출력
            printf("%s %d\n", sectorData, spareVal);
        }

        fclose(flashmemoryfp);
    }

 
    else if (command == 'e') {
        if (argc != 4) return 0;

        char* flashfile = argv[2];
        int pbn = atoi(argv[3]);

        flashmemoryfp = fopen(flashfile, "rb+");
        if (flashmemoryfp == NULL) {
            return 0;
        }

        fdd_erase(pbn);

        fclose(flashmemoryfp);
        }

    else if (command == 'u') {
        if (argc != 6) return 0;

        char* flashfile = argv[2];
        int target_ppn = atoi(argv[3]);
        char* newSectorData = argv[4];
        int newSpareVal = atoi(argv[5]);

        flashmemoryfp = fopen(flashfile, "rb+");
        if (flashmemoryfp == NULL) {
            return 0;
        }

        int read_count = 0, write_count = 0, erase_count = 0;

        // 대상 페이지가 속한 블록과 블록 내 페이지 인덱스 계산
        int target_block = target_ppn / PAGE_NUM;
        int target_page_index = target_ppn % PAGE_NUM;

        // 타겟 페이지가 비어있는지 검사
        char targetPage[PAGE_SIZE];
        if (fdd_read(target_ppn, targetPage) == 1) {
            int isEmpty = 1;
            for (int i = 0; i < SECTOR_SIZE; i++) {
                if ((unsigned char)targetPage[i] != 0xFF) {
                    isEmpty = 0;
                    break;
                }
            }
            if (isEmpty) {
                fclose(flashmemoryfp);
                return 0;
            }
        }
        else {
            fclose(flashmemoryfp);
            return 0;
        }

        // 전체 파일 크기로부터 총 블록 수 계산
        fseek(flashmemoryfp, 0, SEEK_END);
        long filesize = ftell(flashmemoryfp);
        int totalBlocks = filesize / BLOCK_SIZE;

        char buffer[PAGE_NUM][PAGE_SIZE];
        for (int i = 0; i < PAGE_NUM; i++) {
            int ppn = target_block * PAGE_NUM + i;
            if (i == target_page_index) {
                // 업데이트 대상 페이지는 버퍼에 저장하지 않고 0xFF로 초기화
                memset(buffer[i], 0xFF, PAGE_SIZE);
            }
            else {
                if (fdd_read(ppn, buffer[i]) == 1) {
                    read_count++; // 무조건 1회 증가 (총 7회)
                }
                else {
                    memset(buffer[i], 0xFF, PAGE_SIZE);
                    read_count++; // 실패 시에도 증가
                }
            }
        }
        // 이 시점에서 read_count == 7


        int validCount = 0;
        for (int i = 0; i < PAGE_NUM; i++) {
            if (i == target_page_index) continue;
            int hasData = 0;
            for (int j = 0; j < SECTOR_SIZE; j++) {
                if ((unsigned char)buffer[i][j] != 0xFF) {
                    hasData = 1;
                    break;
                }
            }
            if (hasData) validCount++;
        }

        if (validCount == 0) {
            // [갱신할 페이지가 1개인 경우]
            // 타겟 페이지 외에는 모두 빈 페이지이므로 free block 복사/recopy 생략.
            // target block만 erase 후 업데이트 페이지 기록.
            if (fdd_erase(target_block) == 1) {
                erase_count++;  // target block erase (1회)
            }
            char updatedPage[PAGE_SIZE];
            memset(updatedPage, 0xFF, PAGE_SIZE);
            int len = strlen(newSectorData);
            if (len > SECTOR_SIZE) len = SECTOR_SIZE;
            memcpy(updatedPage, newSectorData, len);
            memcpy(updatedPage + SECTOR_SIZE, &newSpareVal, sizeof(int));
            if (fdd_write(target_ppn, updatedPage) == 1) {
                write_count++;  // 업데이트 페이지 write (1회)
            }
            // free block 관련 작업은 하지 않으므로, erase_count는 1회가 됨.
        }
        else {
            // 일반 경우: 타겟 페이지 외에 유효 데이터가 있는 경우
            // 빈 블록(free block) 찾기: 모든 페이지의 섹터 영역이 0xFF인 블록
            int free_block = -1;
            char tempPage[PAGE_SIZE];
            for (int b = 0; b < totalBlocks; b++) {
                if (b == target_block) continue;
                int isEmpty = 1;
                for (int p = 0; p < PAGE_NUM; p++) {
                    if (fdd_read(b * PAGE_NUM + p, tempPage) == 1) {
                        for (int i = 0; i < SECTOR_SIZE; i++) {
                            if ((unsigned char)tempPage[i] != 0xFF) {
                                isEmpty = 0;
                                break;
                            }
                        }
                        if (!isEmpty) break;
                    }
                }
                if (isEmpty) {
                    free_block = b;
                    break;
                }
            }
            if (free_block == -1) {
                fclose(flashmemoryfp);
                return 0;
            }

  
            for (int i = 0; i < PAGE_NUM; i++) {
                if (i == target_page_index) continue; // 타겟 페이지는 복사하지 않음
                int free_ppn = free_block * PAGE_NUM + i;
                int hasData = 0;
                for (int j = 0; j < SECTOR_SIZE; j++) {
                    if ((unsigned char)buffer[i][j] != 0xFF) {
                        hasData = 1;
                        break;
                    }
                }
                if (hasData) {
                    if (fdd_write(free_ppn, buffer[i]) == 1) {
                        write_count++;  // 유효 데이터일 때만 write 카운터 증가
                    }
                }
            }

   
            if (fdd_erase(target_block) == 1) {
                erase_count++;  // target block erase (1회)
            }

   
            for (int i = 0; i < PAGE_NUM; i++) {
                if (i == target_page_index) continue;
                int free_ppn = free_block * PAGE_NUM + i;
                char tempData[PAGE_SIZE];
                if (fdd_read(free_ppn, tempData) == 1) {
                    int hasData = 0;
                    for (int j = 0; j < SECTOR_SIZE; j++) {
                        if ((unsigned char)tempData[j] != 0xFF) {
                            hasData = 1;
                            break;
                        }
                    }
                    if (hasData) {
                        read_count++;  // recopy 시 유효 데이터일 때만 추가 read 카운터 증가
                        int target_ppn2 = target_block * PAGE_NUM + i;
                        if (fdd_write(target_ppn2, tempData) == 1) {
                            write_count++;  // 유효 데이터일 때만 write 카운터 증가
                        }
                    }
                }
            }


            char updatedPage[PAGE_SIZE];
            memset(updatedPage, 0xFF, PAGE_SIZE);
            int len = strlen(newSectorData);
            if (len > SECTOR_SIZE) len = SECTOR_SIZE;
            memcpy(updatedPage, newSectorData, len);
            memcpy(updatedPage + SECTOR_SIZE, &newSpareVal, sizeof(int));
            int target_ppn2 = target_block * PAGE_NUM + target_page_index;
            if (fdd_write(target_ppn2, updatedPage) == 1) {
                write_count++;  // 업데이트 페이지 write (1회)
            }

   

            if (fdd_erase(free_block) == 1) {
                erase_count++;  // free block erase (1회)
            }
        }

        fclose(flashmemoryfp);

        // 최종 연산 횟수 출력
        printf("#reads=%d #writes=%d #erases=%d\n", read_count, write_count, erase_count);
        return 0;
        }



    return 0;
}
