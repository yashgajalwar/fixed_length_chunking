#ifndef VARIABLES_H
#define VARIABLES_H
#include <stdio.h>

struct KeyValue {
    int key;
    unsigned char* value;
};
struct Map{
	struct KeyValue data[12];
	int size;
};

struct Node
{
	FILE *fp;
	int uid;
	char *filepath;
	int size;
	char *hash;
	struct Node *next;
};

struct uid_filemap
{
	int uid;
	char filename[50];
	long int size;
	char filepath[50];
	char hash_OriginalFile[50];
	char hash_folder0[50];
	char hash_folder1[50];
	char hash_folder2[50];
	char hash_folder3[50];
	char hash_folder4[50];
	char hash_folder5[50];
	char hash_folder6[50];
	char hash_folder7[50];
	char hash_parity0[50];
	char hash_parity1[50];
	char hash_parity2[50];
	int padding;
};


#define NUM_DATA 8
#define NUM_PARITY 3
#define BUCKETSIZE 5
#define DIRECTORY_PATH "/home/yashgajalwar/Desktop/erasure/test/"
#define SERVER "localhost"
#define USER "root"
#define PASSWORD "root"
#define DATABASE "erasure_demo"

extern struct Node **arr;
extern int count;
extern struct Map myMap;

extern unsigned char gen[(NUM_DATA + NUM_PARITY) * (NUM_DATA + NUM_PARITY)]; //Generator matrix consist of identity matrix and parity matrix - gen[((8+3),3)]
extern unsigned char g_tbls[(NUM_DATA + NUM_PARITY) * (NUM_DATA + NUM_PARITY) * 32]; //g_tbls is internal structure used in ISA-L. Same as parity matrix.
extern unsigned char *databuffs; //databufs to be allocated using calloc as per file size
extern unsigned char *paritybuffs[NUM_PARITY];	//parities calculated will be stored here
extern unsigned char *datachunks[NUM_DATA]; //data from the chunk files will be stored here


void str_reverse(char []);
void getFileName(char [], char []);
void getFileExtension(char [], char []);
int generateFileUid(char []);
int isDir(const char *);
void calChecksum(char [], char []);


#endif