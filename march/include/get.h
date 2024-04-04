#ifndef GET_H
#define GET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openssl/md5.h>
#include "/home/yashgajalwar/Desktop/erasure/isa-l-master/include/erasure_code.h"
#include "variables.h"
#include <mysql/mysql.h>



// Function prototypes
int get(int, char *, char *, int *, unsigned char **);
int search_Db(int, struct uid_filemap *);
int search_uid(int, struct uid_filemap *);
int search(char *, struct uid_filemap *);
// int isDir(const char *);
// void str_reverse(char []);
// void getFileName(char [], char []);
// void getFileExtension(char [], char []);
// int generateFileUid(char []);
void searchMap(struct Map *, int, unsigned char *, int);
void insert(struct Map *, int, unsigned char *, int);
// void calChecksum(char [], char []);



#endif
