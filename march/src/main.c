/*
Implementing Object Storage-Put,Get,List
*/
//#include <mysql/mysql.h>
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
#include "/home/yashgajalwar/Desktop/erasure/march/include/put.h"
#include "/home/yashgajalwar/Desktop/erasure/march/include/get.h"
#include "/home/yashgajalwar/Desktop/erasure/march/include/list.h"
#include "/home/yashgajalwar/Desktop/erasure/march/include/variables.h"



void menu(char[]);

// int generateUid();
// int put(struct Node **,char[]);
// int get(int,char *,char *,int*,unsigned char **);
// int isDir(const char *);
// int putHash(struct Node **,int,char [],int, char *);
// void display(struct Node*);
// // void list(struct Node **);
// void list(void);
// int addARecord_uid(struct uid_filemap);
// int search(char * filename, struct uid_filemap *);
// int search_uid(int uid, struct uid_filemap *);
// void str_reverse(char *);
// void getFileName(char file_path[50], char file_name[50]);
// void getFileExtension(char file_name[50], char file_extension[10]);
// int generateFileUid(char file_path[100]);
// void calChecksum(char file_path[], char result[]);
// struct Node **arr;
// int count=0;
// void calChecksumArray(unsigned char arr[], size_t size,char checksum[]);
// void insert(struct Map *mapp, int keyy, unsigned char* valuee,int chunk_size);
// void searchMap(struct Map *myMap,int keyy,unsigned char* data_from_map,int chunk_size);
// int addARecord_Db(struct uid_filemap);
// int delete_Db(int);
// void list_Db();
// int search_Db(int uid,struct uid_filemap *);

// struct Map myMap;


int main(void)
{	
	int ch=0;
	int uid=0;
	int succ=0;
	char filepath[100] = {'\0'};
	char targetFilePath[100] = {'\0'};
	int i=0; 
	int j=0;
	int k=0;
	int l=0;
	int m=0;
	int p=0;
	int q=0;
	int flag =0;
	char currentstring[100] = {'\0'};
	char choice[100] = {'\0'};
	char *hash_OriginalFile = NULL;
	char *hash_RegenFile = NULL;
	struct uid_filemap d ;
	setbuf(stdout, NULL);

	//Generating Generator Matrix
	gf_gen_rs_matrix(gen, (NUM_DATA + NUM_PARITY), NUM_DATA);
	//now gen structure contains generator matrix(Identity matrix + Parity matrix) required to compute parities for 8:3 EC

	//Initializing g_tables structure
	ec_init_tables(NUM_DATA, NUM_PARITY, &gen[NUM_DATA * NUM_DATA], g_tbls);

	arr=(struct Node **)calloc(BUCKETSIZE, sizeof(struct Node *));
	
	do
	{
		strcpy(choice,"\0");
		menu(choice);
        
		if(choice[0]=='P'||choice[0]=='p'||choice[0]=='G'||choice[0]=='g' || choice[0]=='D'||choice[0]=='d')
		{
			for(i=0;choice[i]!=' ';i++)
			{
				currentstring[i] = choice[i];
			
			}
		}

		if(strcasecmp(currentstring, "Put") == 0)
		{	
			for(k=i+1, l=0; choice[k]!='\0'; k++, l++)
			{
				filepath[l]=choice[k];
				filepath[l+1]='\0';
			}

			uid=put(arr,filepath);

			if(uid!=-1)
			{
				printf("\n\tFile Split Successfully with UID : %d",uid);
			}
			else
			{
				printf("\n\tPut Operation Failed!!");
			}
		}
		else if(strcasecmp(currentstring, "Get") == 0)
		{
			printf("In the get menu\n");
			flag =0;
			char uid_f[50]="\0";
			
			for(j=i+1,m=0;choice[j]!=' ';j++,m++)
			{
				uid_f[m]=choice[j];	
			}
			
			for(i=0;uid_f[i]!='\0'&&flag ==0;i++)
			{
				if(uid_f[i]<48 || uid_f[i]>57)//it is a char
					flag =1;
					
			}
			
			if(flag ==1)
			{
				search(uid_f,&d);
				printf("\n\tUid of the file is : %d",d.uid);
				uid = d.uid;
			}
			else
			{
				sscanf(uid_f,"%d",&uid);
				search_uid(uid,&d);
				printf("\n\tFile  name is : %s",d.filename);
				strcpy(uid_f,d.filename);	
			}
			
			for(p=j+1,q=0;choice[p]!='\0';p++,q++)
			{
				targetFilePath[q]=choice[p];
			}
			targetFilePath[q+1] = '\0';

			int container_index[NUM_DATA+NUM_PARITY];
			for(int cnt=0;cnt<(NUM_DATA+NUM_PARITY);cnt++){
				container_index[cnt]=0;
			}
			int chunk_size = d.uid;
			unsigned char *container[NUM_DATA+NUM_PARITY];
			for(int cnt=0; cnt<(NUM_DATA+NUM_PARITY); cnt++)
			{
				container[cnt]=(unsigned char *)calloc(chunk_size, sizeof(unsigned char));
			}


			succ=get(uid,targetFilePath,uid_f,container_index,container);
            count=0;
			flag = search(uid_f,&d);
			
			if(succ==-1)
			{
				printf("\n\tGet Operation Failed!!");
				if(fopen(targetFilePath,"r")!=NULL){
					remove(targetFilePath);
				}
			}
			else{
				hash_OriginalFile = (char*) calloc(40, sizeof(char));
				hash_OriginalFile = d.hash_OriginalFile;
				//Calculation checksum of regenerated file
				hash_RegenFile = (char*) calloc(40, sizeof(char)); 

				calChecksum(targetFilePath,hash_RegenFile);
				printf("\n\n\n\tChecksum of Original file : %s", hash_OriginalFile);
				printf("\n\tChecksum of Regenerated file : %s \n", hash_RegenFile);

				if(strcmp(hash_OriginalFile, hash_RegenFile) == 0)
				{
					succ=0;
					printf("\n\tChecksum verification successful! \n");
				}
				else
				{
					printf("\n\tChecksum verification failed! \n");
					succ = -1;
					// remove(targetFilePath);
				}
			}
			if(succ==0)
			{
				printf("\n\tThe File is retrieved successfully!!");
			}

			memset(targetFilePath, '\0', strlen(targetFilePath));
		}
		else if(strcasecmp(choice, "List") == 0)
		{
			printf("\n\tListing the objects : ");
			// list(arr);
			list_Db();
			// list();
			
		}
		else if(strcasecmp(currentstring,"Delete")==0){
			char uid_f[50]="\0";			
			for(j=i+1,m=0;choice[j]!=' ';j++,m++)
			{
				uid_f[m]=choice[j];	
			}
			sscanf(uid_f,"%d",&uid);
			printf("In delete menu \n\n");
			delete_Db(uid);
		}
		strcpy(currentstring,"\0");
		
	}while(strcasecmp(choice,"EXIT")!=0);

	if(strcasecmp(choice,"EXIT")==0)
	{
		printf("\n\tThank you! \n\n");
		printf("\t--------------------------------------------------------------------\n\n");
	}
	
	return 0;
}


void menu(char choice[100])
{	
	printf("\n\n\t-------------------------------------------------------------------");
	printf("\n\tObjectStorage> ");
        printf("\n\t1. Type 'put' and path of file for creating an Object");
        printf("\n\t2. Type 'get', UID and path of target directory for getting an Object");
        printf("\n\t3. Type 'list' for getting the list of all objects in object storage");
		printf("\n\t4. Type 'delete' and UID for deleting a row from database ");
        printf("\n\t5. 'exit'\n");
	printf("\n\t>> ");
	setbuf(stdin,NULL);
	scanf("%[^\n]%*c", choice);
	printf("\t--------------------------------------------------------------------\n");
	
}

