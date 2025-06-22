#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "student.h"

#define HEADER_COUNT_OFFSET 0
#define HEADER_FREE_OFFSET  (sizeof(unsigned int))

int readRecord(FILE *fp, STUDENT *s, int rrn);
void unpack(const char *recordbuf, STUDENT *s);
int writeRecord(FILE *fp, const STUDENT *s, int rrn);
void pack(char *recordbuf, const STUDENT *s);
int append(FILE *fp, char *sid, char *name, char *dept, char *addr, char *email);

void search(FILE *fp, enum FIELD f, char *keyval);
void print(const STUDENT *s[], int n);
enum FIELD getFieldID(char *fieldname);
int delete(FILE* fp, enum FIELD f, char* keyval);
int insert(FILE* fp, char* id, char* name, char* dept, char* addr, char* email);

int main(int argc, char* argv[]) {
    FILE* fp;
    if (argc < 2) {
        fprintf(stderr, "Usage:\n"
            "  %s -a <file> \"sid\" \"name\" \"dept\" \"addr\" \"email\"\n"
            "  %s -s <file> \"FIELD=value\"\n"
            "  %s -d <file> \"FIELD=value\"\n"
            "  %s -i <file> \"field1\" \"field2\" \"field3\" \"field4\" \"field5\"\n",
            argv[0], argv[0], argv[0], argv[0]);
        exit(1);
    }

    if (strcmp(argv[1], "-a") == 0) {
        if (argc != 8) {
            fprintf(stderr, "Error: append requires 5 field arguments\n");
            exit(1);
        }
        fp = fopen(argv[2], "r+b");
        if (!fp) {
            fp = fopen(argv[2], "w+b");
            if (!fp) { perror("fopen"); exit(1); }
            unsigned int initCount = 0;
            int initFree = -1;
            fwrite(&initCount, sizeof(initCount), 1, fp);
            fwrite(&initFree, sizeof(initFree), 1, fp);
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
    else if (strcmp(argv[1], "-d") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Error: delete requires FIELD=value\n");
            exit(1);
        }
        char* arg = argv[3];
        char* eq = strchr(arg, '=');
        if (!eq) { fprintf(stderr, "Invalid delete format\n"); exit(1); }
        *eq = '\0';
        enum FIELD f = getFieldID(arg);
        if (f < 0) { fprintf(stderr, "Unknown field '%s'\n", arg); exit(1); }
        fp = fopen(argv[2], "r+b");
        if (!fp) { perror("fopen"); exit(1); }
        if (!delete(fp, f, eq + 1)) {
            fprintf(stderr, "delete failed or no matching record\n");
            exit(1);
        }
        fclose(fp);
    }
    else if (strcmp(argv[1], "-i") == 0) {
        if (argc != 8) {
            fprintf(stderr, "Error: insert requires 5 field arguments\n");
            exit(1);
        }
        fp = fopen(argv[2], "r+b");
        if (!fp) {
            fp = fopen(argv[2], "w+b");
            if (!fp) { perror("fopen"); exit(1); }
            unsigned int initCount = 0;
            int initFree = -1;
            fwrite(&initCount, sizeof(initCount), 1, fp);
            fwrite(&initFree, sizeof(initFree), 1, fp);
        }
        if (!insert(fp, argv[3], argv[4], argv[5], argv[6], argv[7])) {
            fprintf(stderr, "insert failed\n");
            exit(1);
        }
        fclose(fp);
    }
    else {
        fprintf(stderr, "Unknown option '%s'\n", argv[1]);
        exit(1);
    }
    return 0;
}

void print(const STUDENT *s[], int n) {
    printf("#records = %d\n", n);
    for (int i = 0; i < n; i++) {
        printf("%s#%s#%s#%s#%s#\n",
               s[i]->sid, s[i]->name, s[i]->dept,
               s[i]->addr, s[i]->email);
    }
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




void unpack(const char* recordbuf, STUDENT* s) {
    int idx = 0, pos;
    char tmp[31];
    // SID (8)
    pos = 0;
    while (idx < RECORD_SIZE && recordbuf[idx] != '#' && pos < 8)
        tmp[pos++] = recordbuf[idx++];
    tmp[pos] = '\0';
    while (pos > 0 && tmp[pos - 1] == ' ')
        tmp[--pos] = '\0';
    strncpy(s->sid, tmp, sizeof(s->sid)); s->sid[8] = '\0';
    idx++; // '#'
    // NAME (10)
    pos = 0;
    while (idx < RECORD_SIZE && recordbuf[idx] != '#' && pos < 10)
        tmp[pos++] = recordbuf[idx++];
    tmp[pos] = '\0';
    while (pos > 0 && tmp[pos - 1] == ' ')
        tmp[--pos] = '\0';
    strncpy(s->name, tmp, sizeof(s->name)); s->name[10] = '\0';
    idx++;
    // DEPT (12)
    pos = 0;
    while (idx < RECORD_SIZE && recordbuf[idx] != '#' && pos < 12)
        tmp[pos++] = recordbuf[idx++];
    tmp[pos] = '\0';
    while (pos > 0 && tmp[pos - 1] == ' ')
        tmp[--pos] = '\0';
    strncpy(s->dept, tmp, sizeof(s->dept)); s->dept[12] = '\0';
    idx++;
    // ADDR (30)
    pos = 0;
    while (idx < RECORD_SIZE && recordbuf[idx] != '#' && pos < 30)
        tmp[pos++] = recordbuf[idx++];
    tmp[pos] = '\0';
    while (pos > 0 && tmp[pos - 1] == ' ')
        tmp[--pos] = '\0';
    strncpy(s->addr, tmp, sizeof(s->addr)); s->addr[30] = '\0';
    idx++;
    // EMAIL (20)
    pos = 0;
    while (idx < RECORD_SIZE && recordbuf[idx] != '#' && pos < 20)
        tmp[pos++] = recordbuf[idx++];
    tmp[pos] = '\0';
    while (pos > 0 && tmp[pos - 1] == ' ')
        tmp[--pos] = '\0';
    strncpy(s->email, tmp, sizeof(s->email)); s->email[20] = '\0';
}

int readRecord(FILE* fp, STUDENT* s, int rrn) {
    char recordbuf[RECORD_SIZE];
    if (fseek(fp, HEADER_SIZE + rrn * RECORD_SIZE, SEEK_SET) != 0) return 0;
    if (fread(recordbuf, 1, RECORD_SIZE, fp) < RECORD_SIZE) return 0;
    unpack(recordbuf, s);
    return 1;
}

int writeRecord(FILE* fp, const STUDENT* s, int rrn) {
    char recordbuf[RECORD_SIZE];
    pack(recordbuf, s);
    if (fseek(fp, HEADER_SIZE + rrn * RECORD_SIZE, SEEK_SET) != 0) return 0;
    if (fwrite(recordbuf, 1, RECORD_SIZE, fp) < RECORD_SIZE) return 0;
    fflush(fp);
    return 1;
}

int append(FILE* fp, char* sid, char* name, char* dept, char* addr, char* email) {
    unsigned int recCount;
    int freeHead;
    STUDENT sObj;
    // copy fields
    strncpy(sObj.sid, sid, sizeof(sObj.sid));     sObj.sid[8] = '\0';
    strncpy(sObj.name, name, sizeof(sObj.name)); sObj.name[10] = '\0';
    strncpy(sObj.dept, dept, sizeof(sObj.dept)); sObj.dept[12] = '\0';
    strncpy(sObj.addr, addr, sizeof(sObj.addr)); sObj.addr[30] = '\0';
    strncpy(sObj.email, email, sizeof(sObj.email)); sObj.email[20] = '\0';
    // read header
    fseek(fp, HEADER_COUNT_OFFSET, SEEK_SET);
    if (fread(&recCount, sizeof(recCount), 1, fp) != 1) return 0;
    if (fread(&freeHead, sizeof(freeHead), 1, fp) != 1) return 0;
    // write record at end
    if (!writeRecord(fp, &sObj, recCount)) return 0;
    // update header count, preserve freeHead
    recCount++;
    fseek(fp, HEADER_COUNT_OFFSET, SEEK_SET);
    if (fwrite(&recCount, sizeof(recCount), 1, fp) != 1) return 0;
    if (fwrite(&freeHead, sizeof(freeHead), 1, fp) != 1) return 0;
    fflush(fp);
    return 1;
}

void search(FILE* fp, enum FIELD f, char* keyval) {
    long fileSize;
    int slotCount;
    STUDENT tmp;
    STUDENT** matches;
    int found = 0;
    char recordbuf[RECORD_SIZE];
    // determine total slots
    fseek(fp, 0, SEEK_END);
    fileSize = ftell(fp);
    slotCount = (fileSize - HEADER_SIZE) / RECORD_SIZE;
    matches = malloc(slotCount * sizeof(STUDENT*));
    for (int i = 0; i < slotCount; i++) {
        long offset = HEADER_SIZE + i * RECORD_SIZE;
        fseek(fp, offset, SEEK_SET);
        if (fread(recordbuf, 1, RECORD_SIZE, fp) < RECORD_SIZE) continue;
        if (recordbuf[0] == '*') continue;
        unpack(recordbuf, &tmp);
        char* fieldPtr = NULL;
        switch (f) {
            case SID:   fieldPtr = tmp.sid;   break;
            case NAME:  fieldPtr = tmp.name;  break;
            case DEPT:  fieldPtr = tmp.dept;  break;
            default:    break;
        }
        if (fieldPtr && strcmp(fieldPtr, keyval) == 0) {
            STUDENT* p = malloc(sizeof(STUDENT));
            *p = tmp;
            matches[found++] = p;
        }
    }
    print((const STUDENT**)matches, found);
    for (int i = 0; i < found; i++) free(matches[i]);
    free(matches);
}

int delete(FILE* fp, enum FIELD f, char* keyval) {
    unsigned int recCountOrig;
    int freeHead;
    STUDENT tmp;
    char recordbuf[RECORD_SIZE];
    // read header
    fseek(fp, 0, SEEK_SET);
    if (fread(&recCountOrig, sizeof(recCountOrig), 1, fp) != 1) return 0;
    if (fread(&freeHead, sizeof(freeHead), 1, fp) != 1) return 0;
    int newFreeHead = freeHead;
    // determine total slots
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    int slotCount = (fileSize - HEADER_SIZE) / RECORD_SIZE;
    int deleteCount = 0;
    for (int i = 0; i < slotCount; i++) {
        long offset = HEADER_SIZE + i * RECORD_SIZE;
        fseek(fp, offset, SEEK_SET);
        if (fread(recordbuf, 1, RECORD_SIZE, fp) < RECORD_SIZE) continue;
        if (recordbuf[0] == '*') continue;
        unpack(recordbuf, &tmp);
        char* fieldPtr = NULL;
        switch (f) {
            case SID:   fieldPtr = tmp.sid;   break;
            case NAME:  fieldPtr = tmp.name;  break;
            case DEPT:  fieldPtr = tmp.dept;  break;
            default:    break;
        }
        if (fieldPtr && strcmp(fieldPtr, keyval) == 0) {
            // mark deletion: '*' + next pointer
            fseek(fp, offset, SEEK_SET);
            char mark = '*';
            fwrite(&mark, 1, 1, fp);
            fwrite(&newFreeHead, sizeof(newFreeHead), 1, fp);
            fflush(fp);
            newFreeHead = i;
            deleteCount++;
        }
    }
    if (deleteCount == 0) return 0;
    // update header: decrement count, update free head
    unsigned int recCountNew = recCountOrig - deleteCount;
    fseek(fp, 0, SEEK_SET);
    fwrite(&recCountNew, sizeof(recCountNew), 1, fp);
    fwrite(&newFreeHead, sizeof(newFreeHead), 1, fp);
    fflush(fp);
    return 1;
}

int insert(FILE* fp,
           char* id, char* name, char* dept,
           char* addr, char* email)
{
    unsigned int recCount;
    int freeHead;

    // 1) 헤더 읽기: recCount(4바이트), freeHead(4바이트)
    fseek(fp, 0, SEEK_SET);
    if (fread(&recCount, sizeof(recCount), 1, fp) != 1) return 0;
    if (fread(&freeHead, sizeof(freeHead), 1, fp) != 1) return 0;

    // 2) STUDENT 객체에 사용자 입력 복사
    STUDENT sObj;
    strncpy(sObj.sid,   id,   sizeof(sObj.sid));   sObj.sid[8]   = '\0';
    strncpy(sObj.name,  name, sizeof(sObj.name));  sObj.name[10] = '\0';
    strncpy(sObj.dept,  dept, sizeof(sObj.dept));  sObj.dept[12] = '\0';
    strncpy(sObj.addr,  addr, sizeof(sObj.addr));  sObj.addr[30] = '\0';
    strncpy(sObj.email, email,sizeof(sObj.email)); sObj.email[20]= '\0';

    // 3) 삭제 리스트가 비어 있으면 append 분기
    if (freeHead == -1) {
        // (a) 파일 끝(recCount 위치)에 새 레코드 기록
        if (!writeRecord(fp, &sObj, recCount)) return 0;

        // (b) 헤더: recCount++, freeHead(-1 그대로)
        recCount++;
        fseek(fp, 0, SEEK_SET);
        if (fwrite(&recCount, sizeof(recCount), 1, fp) != 1) return 0;
        if (fwrite(&freeHead, sizeof(freeHead), 1, fp) != 1) return 0;
        fflush(fp);

        return 1;
    }

    // 4) 삭제 리스트 재활용 분기
    //    - freeHead가 가리키는 RRN을 reuseRrn으로 삼고,
    //      그 위치의 ‘*’ 뒤 4바이트를 읽어 다음 freeHead로 사용
    int reuseRrn = freeHead;
    long offset = HEADER_SIZE + reuseRrn * RECORD_SIZE;

    // (a) 다음 freeHead 읽기
    fseek(fp, offset + 1, SEEK_SET);
    if (fread(&freeHead, sizeof(freeHead), 1, fp) != 1) return 0;

    // (b) reuseRrn 위치에 새 레코드 덮어쓰기
    if (!writeRecord(fp, &sObj, reuseRrn)) return 0;

    // (c) 헤더: recCount++, updated freeHead
    recCount++;
    fseek(fp, 0, SEEK_SET);
    if (fwrite(&recCount, sizeof(recCount), 1, fp) != 1) return 0;
    if (fwrite(&freeHead, sizeof(freeHead), 1, fp) != 1) return 0;
    fflush(fp);

    return 1;
}




enum FIELD getFieldID(char* fieldname) {
    if (strcmp(fieldname, "SID") == 0)   return SID;
    if (strcmp(fieldname, "NAME") == 0)  return NAME;
    if (strcmp(fieldname, "DEPT") == 0)  return DEPT;
    if (strcmp(fieldname, "ADDR") == 0)  return ADDR;
    if (strcmp(fieldname, "EMAIL") == 0) return EMAIL;
    return -1;
}

