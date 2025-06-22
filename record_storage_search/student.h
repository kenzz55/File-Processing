#ifndef _STUDENT_H_
#define _STUDENT_H_

// fixed length record ���� ���
#define RECORD_SIZE	85	// sid(8) + name(10) + department(12) + address(30) + email(20) + 5*delimiter
#define HEADER_SIZE	8	// #records(4 bytes) + reserved(4 bytes)

// �ʿ��� ��� 'define'�� �߰��� �� ����.

enum FIELD {SID=0, NAME, DEPT, ADDR, EMAIL};

typedef struct _STUDENT
{
	char sid[9];		// �й�
	char name[11];	// �̸�
	char dept[13];	// �а�
	char addr[31];	// �ּ�
	char email[21];	// �̸��� �ּ�
} STUDENT;

#endif
