#ifndef PUT_H
#define PUT_H

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
int generateUid();
int put(struct Node **, char []);
int putHash(struct Node **, int, char [], int, char *);
int addARecord_uid(struct uid_filemap);
int addARecord_Db(struct uid_filemap);
// void calChecksum(char [], char []);
// int isDir(const char *);
// void getFileName(char [], char []);
// void getFileExtension(char [], char []);
// int generateFileUid(char []);
// void str_reverse(char []);


// Struct definitions
// ... (include all necessary struct definitions)


#endif