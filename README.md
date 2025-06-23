## 파일 I/O 연산
1. **copy.c**: 원본 파일을 10바이트 단위로 읽어 복사본 파일에 저장  
2. **merge.c**: 두 개의 파일을 지정 순서대로 병합하여 새로운 파일 생성  
3. **read.c**: 지정된 오프셋(offset)에서부터 원하는 바이트 수만큼 읽어 화면에 출력  
4. **write.c**: 지정된 오프셋부터 주어진 문자열 데이터로 덮어쓰기  
5. **insert.c**: 지정된 오프셋 위치에 문자열 데이터를 삽입  
6. **delete.c**: 지정된 오프셋에서부터 원하는 바이트 수만큼 데이터 삭제

---

## Flash Device Driver

### 주요 기능
1. **Flash Memory Emulator (옵션 c)**  
   ```bash
   a.out c <flashfile> <#blocks>

   
   ```
- `<flashfile>`: 생성할 플래시 메모리 파일 이름
- `<#blocks>`: 블록 수 (각 블록은 8개의 페이지로 구성)
- `각 페이지`: 512B 섹터 + 16B 스페어, 초기값 `0xFF`

2. **페이지 쓰기 (옵션 w)**  
   ```bash
   a.out w <flashfile> <ppn> "<sectordata>" "<sparedata>"

   
   ```
- `<ppn>`: 물리적 페이지 번호 (0부터 시작)
- `<sectordata>`: 최대 512B 문자열 (나머지 0xFF로 채움)
- `<sparedata>`: 정수 (4바이트 바이너리)

3. **페이지 읽기 (옵션 r)**  
   ```bash
   a.out r <flashfile> <ppn>

   
   ```
- 지정된 페이지의 섹터 및 스페어 데이터 출력
- 섹터: 첫 번째 0xFF 전까지
- 스페어: 첫 4바이트 정수

4. **블록 소거 (옵션 e)**  
   ```bash
   a.out e <flashfile> <ppn>

   
   ```
- `<ppn>`: 물리적 페이지 번호 (0부터 시작)
- 해당 블록의 모든 바이트를 0xFF로 초기화

5. **In-Place Update (옵션 u)**  
   ```bash
   a.out u <flashfile> <ppn> "<sectordata>" "<sparedata>"

   
   ```
- 기존 페이지 갱신 시 In-Place Update 알고리즘 적용
- 수행 후 #reads=<X> #writes=<Y> #erases=<Z> 형식으로 최소 연산 횟수 출력

---
## Hybrid Mapping FTL

### 주요 기능  
`ftlmgr.c` 내에서 Hybrid Mapping 방식으로 FTL을 구현  
(플래시 디바이스 드라이버는 `fdevicedriver.c`, 관련 상수는 `hybridmapping.h`에 정의됨)

1. **초기화 (ftl_open)**  
   - Address Mapping Table 생성 및 초기화  
   - 각 `lbn`에 대응하는 `pbn`, `last_offset`를 모두 `-1`로 설정  
   - Free block 리스트를 **linked list** 형태로 구성 (모든 블록 포함)  
   - 블록 할당/소거 시 리스트 갱신  

2. **쓰기 (ftl_write)**  
   ```c
   void ftl_write(int lsn, char *sectorbuf);
   ```
   - 논리 섹터 번호(`lsn`)를 입력받아 sector 데이터를 페이지 단위로 저장  
   - 적절한 `ppn`을 Hybrid Mapping 기법으로 결정  
   - `fdd_write()` 호출 전, 페이지 버퍼 구성 (sector + spare 영역에 `lsn` 4B 저장)  
   - 블록 내에 빈 페이지 없을 경우 **새 블록 할당 + 데이터 복사 + old 블록 erase**

3. **읽기 (ftl_read)**  
   ```c
   void ftl_read(int lsn, char *sectorbuf);
   ```
   - `lsn`에 대응되는 페이지에서 sector 영역만 읽어 `sectorbuf`에 저장  
   - 페이지 단위로 읽기: `fdd_read()` 호출  
   - spare 영역은 무시  

4. **매핑 테이블 출력 (ftl_print)**  
   ```c
   void ftl_print();
   ```
   - 현재 Address Mapping Table의 `lbn`, `pbn`, `last_offset` 상태 출력  
   - 형식:  
     ```
     lbn pbn last_offset
     0   2   0
     1  -1  -1
     ...
     ```
---
## Fixed-Length Record 저장 및 검색

### 주요 기능  
`student.c` 내에서 고정 길이 레코드 기반 학생 정보 저장 및 검색 기능 구현  

- 각 레코드는 총 5개의 필드로 구성되며, 각 필드는 구분자 `#`로 구분됨  
  - 학번(8), 이름(10), 학과(12), 주소(30), 이메일(20)  
  - 총 85바이트 고정 길이  

- 파일 맨 앞에는 헤더 레코드가 존재  
  - 전체 레코드 수(4B) + 예약공간(4B) = 8바이트  


### 레코드 추가 (append)
```bash
a.out -a <record_file> "학번" "이름" "학과" "주소" "이메일"
```
- 사용자 입력으로부터 5개의 필드값을 받아 레코드를 생성 후 파일에 추가  

예시:
```bash
a.out -a students.dat "20071234" "Gildong Hong" "Computer" "Dongjak-gu, Seoul" "gdhong@ssu.ac.kr"
```


### 레코드 검색 (search)
```bash
a.out -s <record_file> "필드명=값"
```
- 필드명: `SID`, `NAME`, `DEPT` ...  
- 해당 조건을 만족하는 레코드들을 검색하여 출력  

예시:
```bash
a.out -s students.dat "NAME=Gildong"

#records = 2
20201234#Gildong#Computer#Dongjak-gu, Seoul#gd@ssu.ac.kr
20211328#Gildong#Computer#Gawnak-gu, Seoul#gildong@ssu.ac.kr
```

---
## Fixed-Length Record 삽입 및 삭제

### 주요 기능  
`student.c` 내에서 고정 길이 레코드 기반 삽입(insert) 및 삭제(delete) 기능 구현  

- 레코드는 총 85바이트 고정 길이  
- 삭제된 레코드는 첫 바이트에 `"*"`로 표시  
- 삭제된 레코드의 위치는 **삭제 레코드 리스트**로 관리됨  
  - 리스트는 파일 헤더의 예약 공간(4B)에 head rrn 저장  
  - 각 삭제 레코드 내부에 **다음 삭제 rrn**을 저장 (4B)


### 레코드 삭제 (delete)
```bash
a.out -d <record_file> "필드명=값"
```
- 필드명: `SID`, `NAME`, `DEPT` 중 하나 
- 조건에 맞는 모든 레코드를 삭제 처리 (`*`로 마킹 + 삭제 리스트에 추가)  
- 헤더의 전체 레코드 수 1 감소  
- 삭제 리스트 헤드 갱신 (새로 삭제된 레코드 rrn 저장)

예시:
```bash
a.out -d students.dat "SID=20201234"
```


### 레코드 삽입 (insert)
```bash
a.out -i <record_file> "학번" "이름" "학과" "주소" "이메일"
```
- 삭제된 레코드가 존재하면 리스트의 **맨 앞 삭제 공간**에 삽입  
- 없으면 파일 끝에 추가  
- 헤더의 전체 레코드 수 1 증가  
- 삭제 리스트 갱신 (리스트에서 삽입된 노드 제거)

예시:
```bash
a.out -i students.dat "20191234" "GD Hong" "Computer" "Dongjak-gu, Seoul" "gdhong@ssu.ac.kr"
```


### 레코드 검색 (search)
```bash
a.out -s <record_file> "필드명=값"
```
- 삭제되지 않은 레코드만 검색 대상 

---


