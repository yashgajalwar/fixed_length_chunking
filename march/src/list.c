#include "/home/yashgajalwar/Desktop/erasure/march/include/list.h"
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
#include <mysql/mysql.h>


// #define NUM_DATA 8
// #define NUM_PARITY 3
// #define BUCKETSIZE 5
// #define DIRECTORY_PATH "/home/yashgajalwar/Desktop/erasure/test/"
// #define SERVER "localhost"
// #define USER "root"
// #define PASSWORD "root"
// #define DATABASE "erasure_demo"


// unsigned char gen[(NUM_DATA + NUM_PARITY) * (NUM_DATA + NUM_PARITY)]; //Generator matrix consist of identity matrix and parity matrix - gen[((8+3),3)]
// unsigned char g_tbls[(NUM_DATA + NUM_PARITY) * (NUM_DATA + NUM_PARITY) * 32]; //g_tbls is internal structure used in ISA-L. Same as parity matrix.
// unsigned char *databuffs; //databufs to be allocated using calloc as per file size
// unsigned char *paritybuffs[NUM_PARITY];	//parities calculated will be stored here
// unsigned char *datachunks[NUM_DATA]; //data from the chunk files will be stored here

void list(void){
	// In the list function
	int flag=0;//to return success
	int rs=0;
	FILE *fin = NULL;
	struct uid_filemap x ;
	//opening the file in rb+ mode
	fin=fopen("/home/yashgajalwar/Desktop/erasure/uid_file.txt","rb");
	// fin=fopen("/home/yashgajalwar/Desktop/erasure/uid_file.txt","ab+");
	
	if(fin==NULL)
	{
		flag=1;//file not opened
	}
	else{
		do{
			rs=fread(&x,sizeof(struct uid_filemap),1,fin);//reading from file
			if(rs==1)
			{
				printf("\n");//displaying the record
				printf("\n\tUID : %d ",x.uid);
				printf("\n\tFile Name : %s ",x.filename);	
				printf("\n\tSize : %ld ",x.size);	
				printf("\n\tFile Path : %s",x.filepath);
				printf("\n\tChecksum of Original File : %s",x.hash_OriginalFile);
				printf("\n\tChecksum of Chunk 0 : %s",x.hash_folder0);
				printf("\n\tChecksum of Chunk 1 : %s",x.hash_folder1);
				printf("\n\tChecksum of Chunk 2 : %s",x.hash_folder2);
				printf("\n\tChecksum of Chunk 3 : %s",x.hash_folder3);
				printf("\n\tChecksum of Chunk 4 : %s",x.hash_folder4);
				printf("\n\tChecksum of Chunk 5 : %s",x.hash_folder5);
				printf("\n\tChecksum of Chunk 6 : %s",x.hash_folder6);
				printf("\n\tChecksum of Chunk 7 : %s",x.hash_folder7);
				printf("\n\tChecksum of Parity Chunk 0 : %s",x.hash_parity0);
				printf("\n\tChecksum of Parity Chunk 1 : %s",x.hash_parity1);
				printf("\n\tChecksum of Parity Chunk 2 : %s",x.hash_parity2);
				printf("\n\tPadding of Original File:%d",x.padding);			
			}
	   	}while(rs==1);
		fclose(fin);
		fin=NULL;
	}
}



int delete_Db(int uid){
	printf("In delete function\n\n");
	MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;

    conn = mysql_init(NULL);
	if (conn == NULL) {
		fprintf(stderr, "mysql_init() failed\n");
		return 1;
	}

    if (mysql_real_connect(conn, SERVER, USER, PASSWORD, DATABASE, 0, NULL, 0)==NULL) {
		fprintf(stderr, "mysql_real_connect() failed\n");
        mysql_close(conn);
        return 1;
    }

    char query[100];
    sprintf(query, "DELETE FROM tb1 WHERE uid = %d",uid);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "Failed to execute query: Error: %s\n", mysql_error(conn));
        mysql_close(conn);
        return 1;
    }

    printf("Deleted row with ID: %d\n", uid);
    mysql_close(conn);
	return 0;
}



void list_Db(){
	printf("In List function\n\n");
	MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;

    conn = mysql_init(NULL);
	if (conn == NULL) {
		fprintf(stderr, "mysql_init() failed\n");
		return;
	}

    if (mysql_real_connect(conn, SERVER, USER, PASSWORD, DATABASE, 0, NULL, 0)==NULL) {
		fprintf(stderr, "mysql_real_connect() failed\n");
        mysql_close(conn);
        return ;
    }

    char query[100];
    sprintf(query, "SELECT * FROM tb1");

    if (mysql_query(conn, query)) {
        fprintf(stderr, "Failed to execute query: Error: %s\n", mysql_error(conn));
        mysql_close(conn);
        return;
    }
    res = mysql_store_result(conn);

    if (res == NULL) {
        fprintf(stderr, "No data returned.\n");
        mysql_close(conn);
        return;
    }

	MYSQL_FIELD *fields = mysql_fetch_fields(res);

    for (int i = 0; i < mysql_num_fields(res); i++) {
        printf("%s\t\t\t", fields[i].name);
    }
    printf("\n\n\n");
    while ((row = mysql_fetch_row(res))) {
        for (int i = 0; i < mysql_num_fields(res); i++) {
            printf("%s \t\t", row[i] ? row[i] : "NULL");
		}
        printf("\n\n\n");
    }

    mysql_free_result(res);

    mysql_close(conn);
	return;
}