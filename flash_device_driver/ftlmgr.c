#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "flash.h"
// 필요한 경우 헤더파일을 추가한다

FILE *flashmemoryfp;	// fdevicedriver.c에서 사용
extern int fdd_read(int ppn, char *pagebuf);
extern int fdd_write(int ppn, char *pagebuf);
extern int fdd_erase(int ppn);

//
// 이 함수는 FTL의 역할 중 일부분을 수행하는데 물리적인 저장장치 flash memory에 Flash device driver를 이용하여 데이터를
// 읽고 쓰거나 블록을 소거하는 일을 한다 (강의자료 참조).
// flash memory에 데이터를 읽고 쓰거나 소거하기 위해서 fdevicedriver.c에서 제공하는 인터페이스를
// 호출하면 된다. 이때 해당되는 인터페이스를 호출할 때 연산의 단위를 정확히 사용해야 한다.
// 읽기와 쓰기는 페이지 단위이며 소거는 블록 단위이다 (주의: 읽기, 쓰기, 소거를 하기 위해서는 반드시 fdevicedriver.c의 함수를 사용해야 함)
// 갱신은 강의자료의 in-place update를 따른다고 가정한다.
// 스페어영역에 저장되는 정수는 하나이며, 반드시 binary integer 모드로 저장해야 한다.
//
int main(int argc, char *argv[])
{	
    if (argc < 2) {
        // 명령이 없는 경우 종료 (출력 요구 없음)
        return 0;
    }

    char command = argv[1][0];

    //
    // (1) flash memory 파일 생성 및 초기화
    // 사용: a.out c <flashfile> <#blocks>
    // 출력 없음
    //
    if (command == 'c') {
        if (argc != 4) return 0;  // 인자 부족 시 그냥 종료

        char* flashfile = argv[2];
        int numBlocks = atoi(argv[3]);

        // 파일 생성 ("wb+") → 블록 단위 erase로 0xFF 초기화
        flashmemoryfp = fopen(flashfile, "wb+");
        if (flashmemoryfp == NULL) {
            // 파일 열기 실패 → 명세에 출력 요구 없음, 바로 종료
            return 0;
        }
        // 모든 블록 0xFF 초기화
        for (int i = 0; i < numBlocks; i++) {
            fdd_erase(i);  // 각 블록 erase
        }
        fclose(flashmemoryfp);
    }

    //
    // (2) 페이지 쓰기
    // 사용: a.out w <flashfile> <ppn> <sectordata> <sparedata>
    // 출력 없음
    //
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

    //
   // (3) 페이지 읽기
   // 사용: a.out r <flashfile> <ppn>
   // 섹터 데이터(0xFF 전까지)와 스페어 데이터(4바이트 정수) 출력
   // 섹터 영역이 전부 0xFF면 아무것도 출력하지 않음
   //
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

    //
    // (4) 블록 소거 (erase)
    // 사용: a.out e <flashfile> <pbn>
    // 출력 없음
    //
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
        //
        // (5) In-place update
        // 사용: a.out u <flashfile> <ppn> <sectordata> <sparedata>
        // 갱신 과정에서 #reads=... #writes=... #erases=... 출력
        //
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

        // ===============================================================
        // Step 1: 타겟 블럭의 타겟 페이지를 제외한 7개 페이지를 버퍼에 읽기
        // (페이지가 비어있든 데이터가 있든 무조건 read 카운터 증가 -> 7회)
        // ===============================================================
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

        // ===============================================================
        // validCount 계산: 타겟 페이지를 제외한 버퍼 내 유효 데이터 페이지 수
        // ===============================================================
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
            // [일반 경우: 타겟 페이지 외에 유효 데이터가 있는 경우]
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

            // ===============================================================
            // Step 2: 버퍼에 있는 (타겟 페이지 제외) 데이터를 free block으로 복사
            // 유효 데이터가 있는 페이지만 write 카운터 증가
            // ===============================================================
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

            // ===============================================================
            // Step 3: 타겟 블럭 소거 (erase)
            // ===============================================================
            if (fdd_erase(target_block) == 1) {
                erase_count++;  // target block erase (1회)
            }

            // ===============================================================
            // Step 4: free block에 저장된 데이터를 타겟 블럭으로 recopy (타겟 페이지 제외)
            // recopy 시, 유효 데이터가 있는 경우에만 read 카운터와 write 카운터 추가 증가
            // ===============================================================
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

            // ===============================================================
            // Step 5: 업데이트할 타겟 페이지에 새 데이터를 쓰기 (write 카운터 1 증가)
            // ===============================================================
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

            // ===============================================================
            // Step 6: free block 소거
            // ===============================================================
            if (fdd_erase(free_block) == 1) {
                erase_count++;  // free block erase (1회)
            }
        }

        fclose(flashmemoryfp);

        // 최종 연산 횟수 출력
        printf("#reads=%d #writes=%d #erases=%d\n", read_count, write_count, erase_count);
        return 0;
        }


	// flash memory 파일 생성: 위에서 선언한 flashmemoryfp를 사용하여 플래시 메모리 파일을 생성한다. 그 이유는 fdevicedriver.c에서 
	//                 flashmemoryfp 파일포인터를 extern으로 선언하여 사용하기 때문이다.
	// 페이지 쓰기: pagebuf의 섹터와 스페어에 각각 입력된 데이터를 정확히 저장하고 난 후 해당 인터페이스를 호출한다
	// 페이지 읽기: pagebuf를 인자로 사용하여 해당 인터페이스를 호출하여 페이지를 읽어 온 후 여기서 섹터 데이터와
	//                  스페어 데이터를 분리해 낸다
	// memset(), memcpy() 등의 함수를 이용하면 편리하다. 물론, 다른 방법으로 해결해도 무방하다.

    return 0;
}
