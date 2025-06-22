

#ifndef	_HYBRID_MAP_H_
#define	_HYBRID_MAP_H_

#define SECTOR_SIZE			512			
#define SPARE_SIZE			16			
#define PAGE_SIZE			(SECTOR_SIZE+SPARE_SIZE)
#define SECTORS_PER_PAGE		1
#define PAGES_PER_BLOCK		4					// 상수값 수정 가능
#define BLOCK_SIZE			(PAGE_SIZE*PAGES_PER_BLOCK)
#define BLOCKS_PER_DEVICE		16 					// 상수값 수정 가능
#define DATABLKS_PER_DEVICE	(BLOCKS_PER_DEVICE - 1)	// 쓰기연산 시 overwrite가 발생할 때 free block 하나가 필요하며, 
											// 따라서 file system이 사용할 수 있는 가용 메모리는 전체 블록보다 하나 작다.
#define DATAPAGES_PER_DEVICE	(DATABLKS_PER_DEVICE*PAGES_PER_BLOCK)	
							// flash memory에 데이터를 저장할 수 있는 실제 페이지의 수
#endif