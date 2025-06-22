// 주의사항
// 1. hybridmapping.h에 정의되어 있는 상수 변수를 우선적으로 사용해야 함 
// (예를 들면, PAGES_PER_BLOCK의 상수값은 채점 시에 변경할 수 있으므로 반드시 이 상수 변수를 사용해야 함)
// 2. hybridmapping.h에 필요한 상수 변수가 정의되어 있지 않을 경우 본인이 이 파일에서 만들어서 사용하면 됨
// 3. 새로운 data structure가 필요하면 이 파일에서 정의해서 쓰기 바람(hybridmapping.h에 추가하면 안됨)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include "hybridmapping.h"
// 필요한 경우 헤더 파일을 추가하시오.
// 필요한 경우 함수를 추가하시오.
// flashmemoryfp 는 main.c 에서 정의된 전역변수
extern FILE *flashmemoryfp;

// flash device driver 함수들 선언
extern int  fdd_read (int ppn, char *pagebuf);
extern int  fdd_write(int ppn, char *pagebuf);
extern int  fdd_erase(int pbn);
// 1) 논리블록→물리블록 매핑 테이블 항목
typedef struct {
	int pbn;          // 이 lbn에 할당된 pbn, -1이면 아직 할당 안 됨
	int last_offset;  // 해당 pbn에서 마지막으로 쓴 페이지 오프셋 (-1 초기화)
} mapping_entry;

// 2) 프리 블록을 관리하는 단일 연결 리스트 노드
typedef struct free_block_node {
	int pbn;
	struct free_block_node* next;
} free_block_node;

// 전역 변수로 두 자료구조 포인터
mapping_entry* map_table = NULL;
static free_block_node* free_head = NULL;
//
// flash memory를 처음 사용할 때 필요한 초기화 작업, 예를 들면 address mapping table에 대한
// 초기화 등의 작업을 수행한다. 따라서, 첫 번째 ftl_write() 또는 ftl_read()가 호출되기 전에
// file system에 의해 반드시 먼저 호출이 되어야 한다.
//
void ftl_open()
{
	//
	// address mapping table 생성 및 초기화
	// free block linked list 생성 및 초기화
    	// 그 이외 필요한 작업 수행

     // 1) 매핑 테이블 할당 및 초기화
    map_table = malloc(sizeof(mapping_entry) * DATABLKS_PER_DEVICE);
    if (!map_table) {
        fprintf(stderr, "ftl_open: map_table malloc failed\n");
        exit(1);
    }
    for (int lbn = 0; lbn < DATABLKS_PER_DEVICE; lbn++) {
        map_table[lbn].pbn = -1;
        map_table[lbn].last_offset = -1;
    }

    // 2) 프리 블록 리스트 생성 (pbn = 0 .. BLOCKS_PER_DEVICE-1)
    free_head = NULL;
    free_block_node* tail = NULL;
    for (int pbn = 0; pbn < BLOCKS_PER_DEVICE; pbn++) {
        free_block_node* node = malloc(sizeof(free_block_node));
        if (!node) {
            fprintf(stderr, "ftl_open: free block node malloc failed\n");
            exit(1);
        }
        node->pbn = pbn;
        node->next = NULL;
        if (!free_head) {
            free_head = node;
        }
        else {
            tail->next = node;
        }
        tail = node;
    }

	
	return;
}

// pbn 할당: free_head에서 꺼내기
static int alloc_free_block() {
    if (!free_head) return -1;        // 더 이상 여유 블록 없음
    int pbn = free_head->pbn;
    free_block_node* old = free_head;
    free_head = free_head->next;
    free(old);
    return pbn;
}

// 가비지 블록 삽입: erase 후 맨 앞에 추가
static void push_free_block(int pbn) {
    // 반드시 erase 수행
    if (fdd_erase(pbn) < 0) {
        fprintf(stderr, "erase failed on pbn %d\n", pbn);
        exit(1);
    }
    // 새 노드 생성해서 맨 앞에 연결
    free_block_node* node = malloc(sizeof(free_block_node));
    if (!node) {
        perror("malloc");
        exit(1);
    }
    node->pbn = pbn;
    node->next = free_head;
    free_head = node;
}

//
// 이 함수를 호출하는 쪽(file system)에서 이미 sectorbuf가 가리키는 곳에 512B의 메모리가 할당되어 있어야 함
// (즉, 이 함수에서 메모리를 할당 받으면 안됨)
//
void ftl_read(int lsn, char *sectorbuf)
{
    int lbn = lsn / PAGES_PER_BLOCK;
    int pbn = map_table[lbn].pbn;
    int last_off = map_table[lbn].last_offset;
    char pagebuf[PAGE_SIZE];

    // 1) 아직 매핑된 블록이 없으면 읽을 데이터가 없음
    if (pbn < 0) {
        fprintf(stderr, "ftl_read: no block mapped for LBN %d\n", lbn);
        exit(1);
    }

    // 2) 해당 블록의 0..last_off 페이지를 순회하며
    for (int i = last_off; i >= 0; i--) {
        int ppn = pbn * PAGES_PER_BLOCK + i;
        if (fdd_read(ppn, pagebuf) != 1) {
            fprintf(stderr, "ftl_read: read failed ppn=%d\n", ppn);
            exit(1);
        }
        int stored_lsn;
        memcpy(&stored_lsn, pagebuf + SECTOR_SIZE, sizeof(int));

        if (stored_lsn == lsn) {
            memcpy(sectorbuf, pagebuf, SECTOR_SIZE);
            return;
        }
    }
    // 3) 순회 후에도 없으면 에러
    fprintf(stderr, "ftl_read: data for LSN %d not found in PBN %d\n",lsn, pbn);
   
    exit(1);
}

//
// 이 함수를 호출하는 쪽(file system)에서 이미 sectorbuf가 가리키는 곳에 512B의 메모리가 할당되어 있어야 함
// (즉, 이 함수에서 메모리를 할당 받으면 안됨)
//
void ftl_write(int lsn, char *sectorbuf)
{
    int lbn = lsn / PAGES_PER_BLOCK;
    int page_off = lsn % PAGES_PER_BLOCK;
    int curr_pbn = map_table[lbn].pbn;
    int last_off = map_table[lbn].last_offset;
    char pagebuf[PAGE_SIZE];
    if (lsn < 0 || lsn >= DATAPAGES_PER_DEVICE) {
        fprintf(stderr, "ftl_write: invalid LSN %d (valid: 0..%d)\n",
                lsn, DATAPAGES_PER_DEVICE - 1);
        exit(1);
    }    // 1) 아직 블록 할당된 적 없으면 새로 할당
    if (curr_pbn < 0) {
        curr_pbn = alloc_free_block();
        if (curr_pbn < 0) {
            fprintf(stderr, "ftl_write: no free block available\n");
            exit(1);
        }
        map_table[lbn].pbn = curr_pbn;
        map_table[lbn].last_offset = -1;
        last_off = -1;
    }

    // 2) 블록에 빈 페이지가 남아 있는 경우: append write
    if (last_off < PAGES_PER_BLOCK - 1) {
        int new_off = last_off + 1;
        // pagebuf 초기화
        memset(pagebuf, 0xFF, PAGE_SIZE);
        // sector 데이터 복사
        memcpy(pagebuf, sectorbuf, SECTOR_SIZE);
        // spare에 lsn(4B) 저장 (맨 왼쪽)
        memcpy(pagebuf + SECTOR_SIZE, &lsn, sizeof(int));
        // 물리 페이지 번호(ppn) 계산 및 쓰기
        int ppn = curr_pbn * PAGES_PER_BLOCK + new_off;
        if (fdd_write(ppn, pagebuf) != 1) {
            fprintf(stderr, "ftl_write: write failed ppn=%d\n", ppn);
            exit(1);
        }
        map_table[lbn].last_offset = new_off;
        return;
    }

    // 3) 블록이 가득 찬 경우: GC + 재할당
    {
        int old_pbn = curr_pbn;
        // (a) 이전 블록에서 각 page_offset별 가장 최신 페이지 인덱스 찾기
        int newest[PAGES_PER_BLOCK];
        for (int i = 0; i < PAGES_PER_BLOCK; i++) newest[i] = -1;
        char readbuf[PAGE_SIZE];

        for (int i = 0; i < PAGES_PER_BLOCK; i++) {
            int ppn_old = old_pbn * PAGES_PER_BLOCK + i;
            if (fdd_read(ppn_old, readbuf) != 1) {
                fprintf(stderr, "ftl_write: read failed ppn=%d\n", ppn_old);
                exit(1);
            }
            int old_lsn;
            memcpy(&old_lsn, readbuf + SECTOR_SIZE, sizeof(int));
            int old_off = old_lsn % PAGES_PER_BLOCK;
            newest[old_off] = i;  // 덮어쓰여도 가장 나중 인덱스가 남음
        }

        // (b) 새 블록 할당
        int new_pbn = alloc_free_block();
        if (new_pbn < 0) {
            fprintf(stderr, "ftl_write: no free block for GC\n");
            exit(1);
        }
        int new_last = -1;

        // (c) 유효 페이지 복사 (현재 덮어쓸 page_off 제외)
        for (int off = 0; off < PAGES_PER_BLOCK; off++) {
            int idx = newest[off];
            if (idx < 0 || off == page_off) continue;
            int ppn_old = old_pbn * PAGES_PER_BLOCK + idx;
            if (fdd_read(ppn_old, readbuf) != 1) {
                fprintf(stderr, "ftl_write: read failed ppn=%d\n", ppn_old);
                exit(1);
            }
            new_last++;
            int ppn_new = new_pbn * PAGES_PER_BLOCK + new_last;
            if (fdd_write(ppn_new, readbuf) != 1) {
                fprintf(stderr, "ftl_write: write failed ppn=%d\n", ppn_new);
                exit(1);
            }
        }

        // (d) 새로운 데이터 쓰기
        new_last++;
        memset(pagebuf, 0xFF, PAGE_SIZE);
        memcpy(pagebuf, sectorbuf, SECTOR_SIZE);
        memcpy(pagebuf + SECTOR_SIZE, &lsn, sizeof(int));
        {
            int ppn_new = new_pbn * PAGES_PER_BLOCK + new_last;
            if (fdd_write(ppn_new, pagebuf) != 1) {
                fprintf(stderr, "ftl_write: write failed ppn=%d\n", ppn_new);
                exit(1);
            }
        }

        // (e) 이전 블록 지우고 free list에 반환
        push_free_block(old_pbn);

        // (f) 매핑 테이블 갱신
        map_table[lbn].pbn = new_pbn;
        map_table[lbn].last_offset = new_last;
    }

	return;
}

// 
// Address mapping table 등을 출력하는 함수이며, 출력 포맷은 과제 설명서 참조
// 출력 포맷을 반드시 지켜야 하며, 그렇지 않는 경우 채점시 불이익을 받을 수 있음
//
void ftl_print()
{
    // 헤더 출력 (단어 사이에 공백 한 칸)
    printf("lbn pbn last_offset\n");
    // 각 LBN별로 pbn, last_offset 출력 (숫자와 숫자 사이 공백 한 칸)
    for (int lbn = 0; lbn < DATABLKS_PER_DEVICE; lbn++) {
        printf("%d %d %d\n",
            lbn,
            map_table[lbn].pbn,
            map_table[lbn].last_offset);
    }

	return;
}
