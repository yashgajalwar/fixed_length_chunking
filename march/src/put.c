#include "/home/yashgajalwar/Desktop/erasure/march/include/put.h"
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

int generateUid()
{
	static int uid=1;
	return uid++;
}
int put(struct Node **arr,char file_path[100])
{
  	FILE *fp = NULL; // fp - file pointer used to point to file_path
	FILE *fchunk = NULL;// fchunk is a pointer to file_chunk 
	int succ=0; // Flag for memory allocation for new node in a bucket [0 fail, 1 success]
	int file_size = 0;
	int chunk_size = 0;
	char file_name[50]={'\0'};	    
	char chunk_path[100] = {'\0'};
	char file_chunk[100]={'\0'};
	int i=0; // pointer to read file_path
	int uid=-1;// storing unique ID for a datafile
	int rs=0;
	int fd=0;// fd - file descriptor
	int fch=0;
	char file_extension[10]={'\0'};
	char directory[100] = {'\0'};
	int dirSucc = 0;
	int check=0;
	char *hash = NULL ;
	char *hashChunks=NULL;
	int padding=0;
	struct uid_filemap a;
	
	
	// fp is a file pointing at location 'file_path' 	
	fp = fopen(file_path, "a+");
	if(fp == NULL)
	{		
		printf("\n\tFile does not exist!");
	}
	else if(isDir(file_path) == 0)
	{
		printf("\n\tGiven path is a directory.");
	}
	else
	{
		// open is a system call used to open a file
		fd = open(file_path, O_RDONLY | O_CREAT);
		// fseek is used to position the file pointer at the end of file
		fseek(fp, 0L, SEEK_END);
		file_size = ftell(fp);// ftell is used to determine the size of file
		chunk_size = file_size / NUM_DATA;

		//getFileName - Function to retrieve a file name from a file path.
		getFileName(file_path, file_name); 
		//getFileExtension - Function to retrieve file_extension from a file path.
		getFileExtension(file_name, file_extension);
		//generateFileUid - Function to retrieve unique Id for a file.
		uid = generateFileUid(file_path);
					
		// Populating structure 'uid_filemap' into a structure variable 'a'.  
		a.uid=uid;
		strcpy(a.filename ,file_name);
		strcpy(a.filepath,file_path);
		
		//allocating memory to a character array of size 40bytes string and initialising to null value
		hash = (char*) calloc(40, sizeof(char));
		//calCheckSum is used to calculate hash checksum value of a file and store it in a 'hash' variable.
		calChecksum(file_path, hash); 
		strcpy(a.hash_OriginalFile , hash);
		
		// checking if file size(in bytes) is multiple of 8
		if((file_size % NUM_DATA) != 0)
		{
			printf("\n\tSystem Requirement : Filesize should be multiple of %d!", NUM_DATA);
			printf("\n\tCurrent filesize : %d bytes\n", file_size);
			fseek(fp, -1, SEEK_END);
			padding = (NUM_DATA - (file_size%NUM_DATA))%NUM_DATA;
			printf("Padding is %d\n",padding);
			for (int itr = 0; itr < padding; itr++) {
				fputc(0, fp);
    		}
			fseek(fp, 0L, SEEK_END);
			file_size = ftell(fp);
			printf("file_size is multiple of 8 :%d \n",file_size);	
		}
		a.size = file_size;
		a.padding=padding;

		// memory allocation for parity chunks.
		for(i=0; i<NUM_PARITY; i++)
		{
			paritybuffs[i] = (unsigned char *)calloc(chunk_size, sizeof(unsigned char));
		}
		
		// memory allocation for data chunks.
		for(i=0; i<NUM_DATA; i++)
		{
			datachunks[i]=(unsigned char *)calloc(chunk_size, sizeof(unsigned char));
		}
		// buffer memory allocation for ease in copy chunks.
		databuffs=(unsigned char *)calloc(file_size/NUM_DATA, sizeof(unsigned char));//to store data chunks
	
		// putHash - Function used to allocate memory in the respective 
		//bucket for the file with unique 'uid'.
		//putHash returns 1 - memory allocation failed.
		succ=putHash(arr,uid,file_path,file_size, hash);

		if(succ == 1)
		{
			printf("\n\tEntry not added to HashTable");
			return -1;
		}
		else
		{
			printf("\n\tEntry added to HashTable successfully!!");	
		}	    	
		chunk_size = file_size / NUM_DATA;
		printf("\n\tChunk size is = %d bytes\n", chunk_size);
			
		for(i=0; i<NUM_DATA; i++)
		{       
			// 'directory' is used as buffer memory of size 100bytes.
			// sprintf - internal function used to store directory path to be created.
			sprintf(directory, DIRECTORY_PATH"folder%d%c", i, '\0');
			dirSucc = isDir(directory);
			if(dirSucc != 0)// directory path doesn't exist.
			{   // creating a folder along the 'directory' path.
				check = mkdir(directory, 0777); 
			}
			if(check != 0)// validating the folder creation
			{
				printf("\n\t Folder creation failed!!!");
				succ = -1;
			}
			//file_chunk is a buffer of character array used to store the file name to be created			
			sprintf(file_chunk, DIRECTORY_PATH"folder%d/%d_chunk%d.%s%c", i, uid, i, file_extension, '\0'); 
			fchunk=fopen(file_chunk,"w");
						
			if(fchunk == NULL)// validating if chunk is created successfully
			{
				printf("\n\tChunk file could not be created!");
				// if folder is missing we will proceed without saving data into folder
				// but to calculate parities we need data chunks to be filled.
				read(fd, databuffs, chunk_size);
				memcpy(datachunks[i], databuffs, chunk_size);
				// lseek(fd,chunk_size,SEEK_CUR);
				
			}
			else
			{
				// opening 'file_chunk' 
				fch = open(file_chunk, O_WRONLY | O_CREAT);
				// copying data from data file to chunks.
				read(fd, databuffs, chunk_size);
				write(fch, databuffs, chunk_size);
				
				// memory allocation of size 40bytes.
				hashChunks=(char*) calloc(40, sizeof(char));
				memset(hashChunks,'\0',sizeof(hashChunks));
				// calCheckSum - function calculates hash checksum value for a file.
				// storing hash checksum of 'file_chunk' in 'hashChunks'.
				calChecksum(file_chunk, hashChunks); 
				// populating struct uid_filemap with hash checksums of chunk files.
				(i==0)?(strcpy(a.hash_folder0,hashChunks)):"";
				(i==1)?(strcpy(a.hash_folder1,hashChunks)):"";
				(i==2)?(strcpy(a.hash_folder2,hashChunks)):"";
				(i==3)?(strcpy(a.hash_folder3,hashChunks)):"";
				(i==4)?(strcpy(a.hash_folder4,hashChunks)):"";
				(i==5)?(strcpy(a.hash_folder5,hashChunks)):"";
				(i==6)?(strcpy(a.hash_folder6,hashChunks)):"";
				(i==7)?(strcpy(a.hash_folder7,hashChunks)):"";
				// copying data into the datachunks.
				memcpy(datachunks[i], databuffs, chunk_size);
				//closing the files.
				close(fch);
				fclose(fchunk);
				fchunk = NULL;
			}
		}

		//Have to search this function in header files.
		// ec_encode_data_base generates parity chunks.
		ec_encode_data_base(file_size / NUM_DATA, NUM_DATA, NUM_PARITY, g_tbls, datachunks, paritybuffs);          
		printf("\n\n\tParity Chunks Generated \n");
		

		for(i=0; i<NUM_PARITY; i++)
		{       
			//creating directory for parity chunks. 
			sprintf(directory, DIRECTORY_PATH"parity%d%c", i, '\0');
			dirSucc = isDir(directory);
			
			if(dirSucc != 0)// create directory 
			{
				check = mkdir(directory, 0777); 
			}
			if(check != 0)// Validate parity folder creation. 
			{
				printf("\n\t Parity Folder creation failed!!!");
				succ = -1;
			}
			// file_chunks is a buffer of character array used to store the parity file name.
			sprintf(file_chunk, DIRECTORY_PATH"parity%d/%d_parity%d.%s%c", i, uid, i, file_extension, '\0');
			//fchunk points to the parity chunk.
			fchunk=fopen(file_chunk,"w");
				
			if(fchunk == NULL)
			{
				printf("\n\tParity file could not be created!");
			}
			else
			{       
				// write the parity chunks from the 'paritybuffs'.
				fch = open(file_chunk, O_RDWR | O_CREAT);
				write(fch, paritybuffs[i], chunk_size);

				//printf("\n\tParity Chunk %d : %s", i, paritybuffs[i]);
				hashChunks=(char*) calloc(40, sizeof(char));
				memset(hashChunks,'\0',sizeof(hashChunks));
				// calculating hash checksum for parity file.
				calChecksum(file_chunk, hashChunks); 
				//propogating hash checksum value in struct uid_filemap.
				(i==0)?(strcpy(a.hash_parity0,hashChunks)):"";
				(i==1)?(strcpy(a.hash_parity1,hashChunks)):"";
				(i==2)?(strcpy(a.hash_parity2,hashChunks)):"";
				//closing files
				close(fch);
				fclose(fchunk);
				fchunk = NULL;
			}
		}   	
		//addARecord_uid - function to create a binary file to store the hash checksum of file chunked,
		// chunk data files, chunk parity files,file_path.
		addARecord_uid(a);
		addARecord_Db(a);
		
		close(fd);
		fclose(fp);
		fp = NULL;
	}
	return uid;   // return the uid [Fail=-1]  	
}

int putHash(struct Node **arr,int uid,char filepath[100],int file_size, char *hash)
{
	int bucket_no;
	int i = 0;
	struct Node *newnode;
	bucket_no = uid % BUCKETSIZE;
	if(arr[bucket_no]==NULL)
	{
		newnode = (struct Node*)calloc(1, sizeof(struct Node));
		if(newnode==NULL)
		{
			printf("\n\tMemory allocation for newnode failed");
			return 1;
		}	
		else
		{
			newnode->filepath=(char*)calloc(strlen(filepath),sizeof(char));			
			strcpy(newnode->filepath,filepath);			
			newnode->uid=uid;
			newnode->size=file_size;
			newnode->hash=(char*)calloc(33,sizeof(char)); // Maximum size of md5 hash = 32 bytes
			strcpy(newnode->hash,hash);
			newnode->next=NULL;
		}
		arr[bucket_no]=newnode;
	}
	else
	{
		struct Node *temp=NULL;
		temp=(arr[bucket_no]);
		while(temp->next!=NULL)
		{
			temp=temp->next;
		}
		newnode= (struct Node*)calloc(1, sizeof(struct Node));
		if(newnode==NULL)
		{
			printf("\n\tMemory allocation for newnode failed");
			return 1;
		}	
		else
		{
			newnode->filepath=(char*)calloc(strlen(filepath),sizeof(char));
			for(i=0;filepath[i]!='\0';i++)
			{
				newnode->filepath[i]=filepath[i];
			}
			newnode->uid=uid;
			newnode->size=file_size;
			newnode->next=NULL;
			temp->next=newnode;
		}
	}
	return 0;
}


void calChecksum(char file_path[], char checksum[])
{
 	char command[100] = {'\0'}; 
 	int i=0;
 	int j=0;
 	int k=0;
 	char result[100] = {'\0'};

	strcpy(command, "cksum ");
	strcat(command, file_path);
 	FILE *ls = popen(command, "r");
 	
 	while(fread(&result[i], sizeof(result[i]), 1, ls))
 	{
 		i++;
 	}
 	
 	for(j=0; result[j] != ' '; j++)
        {
        	checksum[k] = result[j];
        	k++;
        }

}

int addARecord_uid(struct uid_filemap s)
{
	int flag=0;//to return success
	int rs=0;
	FILE *fout=NULL;//file pointer
	FILE *fin = NULL;
	struct uid_filemap x ;
	//opening the file in rb+ mode
	fout=fopen("/home/yashgajalwar/Desktop/erasure/uid_file.txt","ab+");
	// fin=fopen("/home/yashgajalwar/Desktop/erasure/uid_file.txt","rb");
	fin=fopen("/home/yashgajalwar/Desktop/erasure/uid_file.txt","ab+");
	if(fout==NULL)
	{
		flag=1;//file not opened
	}
	else
	{
		fseek(fout,0,SEEK_END);//going to that location
		fwrite(&s,sizeof(struct uid_filemap),1,fout);//writing the record    
		fclose(fout);
		fout=NULL;	
		do
		{
			rs=fread(&x,sizeof(struct uid_filemap),1,fin);//reading from file
			if(rs==1 && x.uid==s.uid)
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
	return flag;
}

int addARecord_Db(struct uid_filemap s)
{
	MYSQL *conn;
		MYSQL_RES *res;
		MYSQL_ROW row;

		conn = mysql_init(NULL);
		if (conn == NULL) {
			fprintf(stderr, "mysql_init() failed\n");
			return 1;
		}

		if (mysql_real_connect(conn, SERVER, USER, PASSWORD, DATABASE, 0, NULL, 0) == NULL) {
			fprintf(stderr, "mysql_real_connect() failed\n");
			mysql_close(conn);
			return 1;
		}
		char query[1024]; 
		sprintf(query,"INSERT INTO tb1 (uid, filename, size, filepath, hashOriginalFile, hash_folder0, hash_folder1, hash_folder2, hash_folder3, hash_folder4, hash_folder5, hash_folder6, hash_folder7, hash_parity0, hash_parity1, hash_parity2, padding) VALUES (%d, '%s', %ld, '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', %d)",s.uid,s.filename,s.size,s.filepath,s.hash_OriginalFile,s.hash_folder0,s.hash_folder1,s.hash_folder2,s.hash_folder3,s.hash_folder4,s.hash_folder5,s.hash_folder6,s.hash_folder7,s.hash_parity0,s.hash_parity1,s.hash_parity2,s.padding);
		if (mysql_query(conn, query) != 0) {
        	fprintf(stderr, "mysql_query() failed (555): %s\n", mysql_error(conn));
			mysql_close(conn);
			return 1;
		}
		printf("Data inserted successfully!\n");

    mysql_close(conn);


}


int isDir(const char *filePath){
	DIR *directory=opendir(filePath);
	if(directory!=NULL)
	{
		closedir(directory);
		return 0;//is a directory
	}
	if(errno=ENOTDIR){
		return 1;
	}
	return -1;
}




void getFileName(char file_path[50], char file_name[50])
{
	int j=0;
	int k=0;
	str_reverse(file_path);
	for(j=0,k=0;file_path[j]!='/' ;j++,k++)
	{
		file_name[k]=file_path[j];
		file_name[k+1] = '\0';
	}
	file_name[k+1] = '\0';
	str_reverse(file_name);
	str_reverse(file_path);
}

void getFileExtension(char file_name[50], char file_extension[50])
{
	int j=0;
	int k=0;
	str_reverse(file_name);
	for(j=0,k=0;file_name[j]!='.' ;j++,k++)
	{
		file_extension[k]=file_name[j];
		file_extension[k+1] = '\0';
	}
	file_extension[k+1] = '\0';
	str_reverse(file_name);
	str_reverse(file_extension);
}
int generateFileUid(char file_path[100])
{
	char result[100] = {'\0'};
 	char command[100] = {'\0'};
	int i=0;
	int j=0;
	int k=0;
	int uid_int=0; 
	char uid[10]={'\0'};

	strcpy(command, "ls -i ");
	strcat(command, file_path);
 	FILE *ls = popen(command, "r");
 	
 	while(fread(&result[i], sizeof(result[i]), 1, ls))
 	{
 		i++;
 	}    
	for(j=0; result[j] != ' '; j++)
	{
		uid[k] = result[j];
		k++;
	}
	uid_int = atoi(uid);
	return uid_int;   
}

void str_reverse(char fstr[50])
{
    int i=0 ;
    int j=0 ;
    int cnt=0 ;
    char fstr2[50];
    
    cnt = strlen(fstr) ;
    for(i=cnt-1, j=0 ; i>=0 ; i--, j++)
    {
        fstr2[j] = fstr[i] ;
        fstr2[j+1] = '\0' ;
    }  
    strcpy(fstr, fstr2);
} 


