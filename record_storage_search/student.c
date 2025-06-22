#include <stdio.h>		
#include "student.h"
#include <stdlib.h>
#include <string.h>

int readRecord(FILE *fp, STUDENT *s, int rrn);
void unpack(const char *recordbuf, STUDENT *s);


int writeRecord(FILE *fp, const STUDENT *s, int rrn);
void pack(char *recordbuf, const STUDENT *s);


int append(FILE *fp, char *sid, char *name, char *dept, char *addr, char *email);


void search(FILE *fp, enum FIELD f, char *keyval);
void print(const STUDENT *s[], int n);


enum FIELD getFieldID(char *fieldname);

void main(int argc, char* argv[]) {
    FILE* fp;

    if (argc < 2) {
        fprintf(stderr, "Usage:\n"
            "  %s -a <file> \"sid\" \"name\" \"dept\" \"addr\" \"email\"\n"
            "  %s -s <file> \"FIELD=value\"\n", argv[0], argv[0]);
        exit(1);
    }

    if (strcmp(argv[1], "-a") == 0) {
        if (argc != 8) {
            fprintf(stderr, "Error: append requires 5 field arguments\n");
            exit(1);
        }
        // 파일이 없으면 생성 및 헤더 초기화
        fp = fopen(argv[2], "r+b");
        if (!fp) {
            fp = fopen(argv[2], "w+b");
            if (!fp) { perror("fopen"); exit(1); }
            unsigned int zero = 0;
            fwrite(&zero, sizeof(zero), 1, fp);
            fwrite(&zero, sizeof(zero), 1, fp);
        }
        if (!append(fp, argv[3], argv[4], argv[5], argv[6], argv[7])) {
            fprintf(stderr, "append failed\n");
            exit(1);
        }
        fclose(fp);

    }
    else if (strcmp(argv[1], "-s") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Error: search requires FIELD=value\n");
            exit(1);
        }
        // FIELD와 value 분리
        char* arg = argv[3];
        char* eq = strchr(arg, '=');
        if (!eq) { fprintf(stderr, "Invalid search format\n"); exit(1); }
        *eq = '\0';
        enum FIELD f = getFieldID(arg);
        if (f < 0) { fprintf(stderr, "Unknown field '%s'\n", arg); exit(1); }

        fp = fopen(argv[2], "r+b");
        if (!fp) { perror("fopen"); exit(1); }
        search(fp, f, eq + 1);
        fclose(fp);

    }
    else {
        fprintf(stderr, "Unknown option '%s'\n", argv[1]);
        exit(1);
    }
}

void print(const STUDENT *s[], int n)
{
	printf("#records = %d\n", n);
	for(int i = 0; i < n; i++)
	{
		printf("%s#%s#%s#%s#%s#\n", s[i]->sid, s[i]->name, s[i]->dept, s[i]->addr, s[i]->email);
	}
}


// recordbuf에 들어온 한 건의 레코드를 STUDENT 구조체로 분해
void unpack(const char* recordbuf, STUDENT* s) {
    int idx = 0, pos;
    char tmp[31];  // 가장 긴 필드(30) + 널

    // SID
    pos = 0;
    while (idx < RECORD_SIZE && recordbuf[idx] != '#' && pos < 8)
        tmp[pos++] = recordbuf[idx++];
    tmp[pos] = '\0';
    while (pos > 0 && tmp[pos - 1] == ' ')
        tmp[--pos] = '\0';
    strncpy(s->sid, tmp, sizeof(s->sid));
    idx++;  // '#'

    // NAME
    pos = 0;
    while (idx < RECORD_SIZE && recordbuf[idx] != '#' && pos < 10)
        tmp[pos++] = recordbuf[idx++];
    tmp[pos] = '\0';
    while (pos > 0 && tmp[pos - 1] == ' ')
        tmp[--pos] = '\0';
    strncpy(s->name, tmp, sizeof(s->name));
    idx++;

    // DEPT
    pos = 0;
    while (idx < RECORD_SIZE && recordbuf[idx] != '#' && pos < 12)
        tmp[pos++] = recordbuf[idx++];
    tmp[pos] = '\0';
    while (pos > 0 && tmp[pos - 1] == ' ')
        tmp[--pos] = '\0';
    strncpy(s->dept, tmp, sizeof(s->dept));
    idx++;

    // ADDR
    pos = 0;
    while (idx < RECORD_SIZE && recordbuf[idx] != '#' && pos < 30)
        tmp[pos++] = recordbuf[idx++];
    tmp[pos] = '\0';
    while (pos > 0 && tmp[pos - 1] == ' ')
        tmp[--pos] = '\0';
    strncpy(s->addr, tmp, sizeof(s->addr));
    idx++;

    // EMAIL
    pos = 0;
    while (idx < RECORD_SIZE && recordbuf[idx] != '#' && pos < 20)
        tmp[pos++] = recordbuf[idx++];
    tmp[pos] = '\0';
    while (pos > 0 && tmp[pos - 1] == ' ')
        tmp[--pos] = '\0';
    strncpy(s->email, tmp, sizeof(s->email));
}

// STUDENT 구조체를 기록용 버퍼(recordbuf)로 포장
void pack(char* recordbuf, const STUDENT* s) {
    int idx = 0, len;

    // SID (8)
    len = strlen(s->sid);
    for (int i = 0; i < 8; i++)
        recordbuf[idx++] = (i < len ? s->sid[i] : ' ');
    recordbuf[idx++] = '#';

    // NAME (10)
    len = strlen(s->name);
    for (int i = 0; i < 10; i++)
        recordbuf[idx++] = (i < len ? s->name[i] : ' ');
    recordbuf[idx++] = '#';

    // DEPT (12)
    len = strlen(s->dept);
    for (int i = 0; i < 12; i++)
        recordbuf[idx++] = (i < len ? s->dept[i] : ' ');
    recordbuf[idx++] = '#';

    // ADDR (30)
    len = strlen(s->addr);
    for (int i = 0; i < 30; i++)
        recordbuf[idx++] = (i < len ? s->addr[i] : ' ');
    recordbuf[idx++] = '#';

    // EMAIL (20)
    len = strlen(s->email);
    for (int i = 0; i < 20; i++)
        recordbuf[idx++] = (i < len ? s->email[i] : ' ');
    recordbuf[idx++] = '#';
}

// rrn 위치의 레코드를 읽어서 s에 저장
int readRecord(FILE* fp, STUDENT* s, int rrn) {
    char recordbuf[RECORD_SIZE];
    if (fseek(fp, HEADER_SIZE + rrn * RECORD_SIZE, SEEK_SET) != 0)
        return 0;
    if (fread(recordbuf, 1, RECORD_SIZE, fp) < RECORD_SIZE)
        return 0;
    unpack(recordbuf, s);
    return 1;
}

// s를 포장하여 rrn 위치에 기록
int writeRecord(FILE* fp, const STUDENT* s, int rrn) {
    char recordbuf[RECORD_SIZE];
    pack(recordbuf, s);
    if (fseek(fp, HEADER_SIZE + rrn * RECORD_SIZE, SEEK_SET) != 0)
        return 0;
    if (fwrite(recordbuf, 1, RECORD_SIZE, fp) < RECORD_SIZE)
        return 0;
    fflush(fp);
    return 1;
}

// 파일 헤더에서 레코드 수를 읽고, 새로운 레코드를 추가한 뒤 헤더 갱신
int append(FILE* fp, char* sid, char* name, char* dept, char* addr, char* email) {
    unsigned int recCount;
    STUDENT sObj;

    // STUDENT 구조체에 인자 복사
    strncpy(sObj.sid, sid, sizeof(sObj.sid));     sObj.sid[8] = '\0';
    strncpy(sObj.name, name, sizeof(sObj.name)); sObj.name[10] = '\0';
    strncpy(sObj.dept, dept, sizeof(sObj.dept)); sObj.dept[12] = '\0';
    strncpy(sObj.addr, addr, sizeof(sObj.addr)); sObj.addr[30] = '\0';
    strncpy(sObj.email, email, sizeof(sObj.email)); sObj.email[20] = '\0';

    // 헤더에서 #records 읽기
    fseek(fp, 0, SEEK_SET);
    if (fread(&recCount, sizeof(unsigned int), 1, fp) != 1)
        return 0;
    // reserved(4B) 스킵
    fseek(fp, 4, SEEK_CUR);

    // 신규 레코드 쓰기
    if (!writeRecord(fp, &sObj, recCount))
        return 0;

    // 헤더에 레코드 수 +1
    recCount++;
    fseek(fp, 0, SEEK_SET);
    if (fwrite(&recCount, sizeof(unsigned int), 1, fp) != 1)
        return 0;
    // reserved는 그대로 0으로 두거나 덮어쓰기
    unsigned int reserved = 0;
    if (fwrite(&reserved, sizeof(unsigned int), 1, fp) != 1)
        return 0;

    return 1;
}

// f 필드가 keyval과 일치하는 모든 레코드를 찾아 print 호출
void search(FILE* fp, enum FIELD f, char* keyval) {
    unsigned int recCount;
    STUDENT tmp;
    STUDENT** matches;
    int found = 0;

    // 헤더에서 레코드 개수 읽기
    fseek(fp, 0, SEEK_SET);
    fread(&recCount, sizeof(unsigned int), 1, fp);
    fseek(fp, 4, SEEK_CUR);

    matches = malloc(recCount * sizeof(STUDENT*));
    for (unsigned int i = 0; i < recCount; i++) {
        if (!readRecord(fp, &tmp, i)) continue;
        // 비교할 필드 포인터 선택
        char* fieldPtr = NULL;
        switch (f) {
        case SID:   fieldPtr = tmp.sid;   break;
        case NAME:  fieldPtr = tmp.name;  break;
        case DEPT:  fieldPtr = tmp.dept;  break;
        case ADDR:  fieldPtr = tmp.addr;  break;
        case EMAIL: fieldPtr = tmp.email; break;
        }
        if (fieldPtr && strcmp(fieldPtr, keyval) == 0) {
            STUDENT* p = malloc(sizeof(STUDENT));
            *p = tmp;
            matches[found++] = p;
        }
    }

    // 결과 출력
    print((const STUDENT**)matches, found);

    // 메모리 해제
    for (int i = 0; i < found; i++) free(matches[i]);
    free(matches);
}

// 문자열을 enum FIELD로 변환
enum FIELD getFieldID(char* fieldname) {
    if (strcmp(fieldname, "SID") == 0)   return SID;
    if (strcmp(fieldname, "NAME") == 0)  return NAME;
    if (strcmp(fieldname, "DEPT") == 0)  return DEPT;
    if (strcmp(fieldname, "ADDR") == 0)  return ADDR;
    if (strcmp(fieldname, "EMAIL") == 0) return EMAIL;
    return -1;
}