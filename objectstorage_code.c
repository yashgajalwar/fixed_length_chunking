/*
Implementing Object Storage-Put,Get,List
*/

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
#include "/home/yashgajalwar/Desktop/isa-l-master/include/erasure_code.h"

#define NUM_DATA 8
#define NUM_PARITY 3
#define BUCKETSIZE 5
#define DIRECTORY_PATH "/home/yashgajalwar/Desktop/test/"

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
};

int generateUid();
void menu(char[]);
int put(struct Node **,char[]);
int get(int,char *,char *);
int isDir(const char *);
int putHash(struct Node **,int,char [],int, char *);
void display(struct Node*);
void list(struct Node **);
int addARecord_uid(struct uid_filemap);
int search(char * filename, struct uid_filemap *);
int search_uid(int uid, struct uid_filemap *);
void str_reverse(char *);
void getFileName(char file_path[50], char file_name[50]);
void getFileExtension(char file_name[50], char file_extension[10]);
int generateFileUid(char file_path[100]);
void calChecksum(char file_path[], char result[]);
struct Node **arr;


unsigned char gen[(NUM_DATA + NUM_PARITY) * (NUM_DATA + NUM_PARITY)]; //Generator matrix consist of identity matrix and parity matrix - gen[((8+3),3)]
unsigned char g_tbls[(NUM_DATA + NUM_PARITY) * (NUM_DATA + NUM_PARITY) * 32]; //g_tbls is internal structure used in ISA-L. Same as parity matrix.
unsigned char *databuffs; //databufs to be allocated using calloc as per file size
unsigned char *paritybuffs[NUM_PARITY];	//parities calculated will be stored here
unsigned char *datachunks[NUM_DATA]; //data from the chunk files will be stored here

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

		if(choice[0]=='P'||choice[0]=='p'||choice[0]=='G'||choice[0]=='g')
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

			
			succ=get(uid,targetFilePath,uid_f);

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
					remove(targetFilePath);
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
			list(arr);
			
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

int generateUid()
{
	static int uid=1;
	return uid++;
}

void menu(char choice[100])
{	
	printf("\n\n\t-------------------------------------------------------------------");
	printf("\n\tObjectStorage> ");
        printf("\n\t1. Type 'put' and path of file for creating an Object");
        printf("\n\t2. Type 'get', UID and path of target directory for getting an Object");
        printf("\n\t3. Type 'list' for getting the list of all objects in object storage");
        printf("\n\t4. 'exit'\n");
	printf("\n\t>> ");
	setbuf(stdin,NULL);
	scanf("%[^\n]%*c", choice);
	printf("\t--------------------------------------------------------------------\n");
	
}

int get(int uid,char *targetFilePath,char * uid_f)
{	
	FILE *fchunk=NULL;// fchunk is file pointer used to read/write file data.
	FILE *ft = NULL; // ft is file pointer used to read/write file data.
	int succ=0;// succ indicates success of 'get' function. [1 successful,0 failure]
	char cc = '\0';
	char *newFilePath = NULL;
	char chunk[100] = {'\0'};// chunk is character array used to store the file path.
	int flag=0;// flag is used to store the result of search function, which indicates whether a file in directory is found.
	int flag2=0;// flag 2 is used as buffer to store state of variables.
	int missingChunks=0;
	int missingParity=0;
	int i=0;// iterator
	int j=0;
	int k=0;
	int r=0;
	char file_name[50]={'\0'};	
	char file_path[50]={'\0'};
	char int_str[20]={'\0'};// int_str is a character array to store the uid of the file.
	char targetPath_old[50] = {'\0'};// targetPath_old stores the directory path.
	int fch = 0;
	int fd = 0;// file descriptor used to access open files.
	int chunk_size = 0;
	int file_size = 0;
	char directory[100] = {'\0'};
	int dirSucc = 0;
	int check=0;
	char file_extension[10]={'\0'};					
	int availableChunks[NUM_DATA + NUM_PARITY] = {0};// integer array to indicate the chunk and parity number need to be recovered or not.  
	unsigned char recovery[NUM_DATA * NUM_DATA];
	unsigned char decode[NUM_DATA * NUM_DATA];
	unsigned char frdm[NUM_DATA * NUM_PARITY];
	unsigned char r_tables[(NUM_DATA + NUM_PARITY) * (NUM_DATA + NUM_PARITY) * 32];
	unsigned char *temp_buffs[NUM_PARITY] = {NULL};
	unsigned char *available_buffs[NUM_DATA] = {NULL};
	unsigned char *buff = NULL;
	int marr[NUM_PARITY] = {0}; // marr is a integer array that stores the lost chunk file numbers.
	char *hash_OriginalFile = NULL;
	char *hash_RegenFile = NULL;
	char *hash_ChunkOriginal = NULL;// buffer to store the hash checksum of original chunk file.
	char *hash_ChunkRegen = NULL;// buffer to store the hash checksum of chunk file.
	struct Node *temp = arr[uid % BUCKETSIZE];	


	//array to set all the parity bits.
	for(i = 0 ; i < NUM_PARITY ; ++i)
	{
		marr[i] = -1 ;
	}	

	struct uid_filemap d;

	// search function searches file name 'uid_f' and store all the chunk block values and information in struct uid_filemap d;
	flag = search(uid_f,&d);

	//initializing variables
	file_size = d.size;
	strcpy(file_path , d.filepath);
	hash_OriginalFile = d.hash_OriginalFile;

	/*while(temp != NULL)
    	{
    		if(temp->uid == uid)
    		{
    			//strcpy(file_path, temp->filepath);
			//file_size = temp->size ;
			hash_OriginalFile = temp->hash ;
			
    		}
    		temp = temp->next;
    	}*/

	if(flag == -1)// checking if file exists.
	{
		printf("\n\tFile with unique ID %d does not exist in the system!", uid);
		succ = -1 ;	
	}
	else if((file_size % NUM_DATA) != 0)// checking if file size is multiple of NUM_DATA;
	{
		printf("\n\tSystem Requirement : Filesize should be multiple of %d!", NUM_DATA);
		printf("\n\tCurrent filesize : %d bytes \n", file_size);
		succ = -1 ;
	}
	else
	{
		// extracting file name from file path... 
		flag = 0;	//reset flag
		for(i=0; file_path[i] != '\0'; i++)
		{
			if(file_path[i] == '/')
			{
				flag2=1;
			}	
		}
		
		if(flag2 == 0)// file path is same as file name.
		{
			strcpy(file_name, file_path);
			printf("%s", file_name);
		}
		else 
		{
			str_reverse(file_path);
			for(j=0,k=0;file_path[j]!='/' ;j++,k++)
			{
				file_name[k]=file_path[j];
			}
			file_name[k+1] = '\0';
			str_reverse(file_name);
			str_reverse(file_path);
		}

		getFileExtension(file_name, file_extension);
		flag2 = 0 ;	// reset buffer flag
		
		chunk_size = file_size / NUM_DATA ;

		// why these 2 lines are needed?
		buff=(unsigned char *)calloc(chunk_size + NUM_DATA, sizeof(unsigned char));
		strcpy(targetPath_old, targetFilePath);	

		do
		{	
			// check if targetFilePath is directory.
			if(isDir(targetFilePath)==0)
			{
				
				flag=0;
				
				strcat(targetFilePath, file_name);
				sprintf(int_str,"%d%c", uid, '\0');
				
				//open the 	targetFilePath in append mode.
				ft = fopen(targetFilePath,"a");

				if(ft == NULL)// failed to open the target file.
				{
					printf("\n\tTarget file could not be opened!!");
					succ = -1;
				}
				else
				{	// file descriptor used to read & write the targetFilePath.
					fd = open(targetFilePath, O_RDWR | O_CREAT);
					for(i = 0; i < NUM_DATA ; ++i)
					{
						//creating a folder for the chunk files.
						sprintf(directory, DIRECTORY_PATH"folder%d%c", i, '\0');
						dirSucc = isDir(directory);
						
						if(dirSucc != 0)
						{
							check = mkdir(directory, 0777); 
						}
						if(check != 0)// Failed to create folder, terminate get function. 
						{
							printf("\n\t Folder creation failed!!!");
							succ = -1;
						}

						// storing file name in 'chunk' character array.
						sprintf(chunk, DIRECTORY_PATH"folder%d/%d_chunk%d.%s%c", i, uid, i, file_extension, '\0');
						fchunk = fopen(chunk, "r") ;

						if(fchunk == NULL)// id chunk file is not found
						{
							availableChunks[i] = 0 ;	
						}
						else
						{
							// copying the chunk into target file.
							fch = open(chunk, O_RDONLY | O_CREAT);
							read(fch, buff, chunk_size);
							write(fd, buff, chunk_size);

							//memory allocation of 40 bytes for storing hash checksum value for chunk file.
							hash_ChunkRegen = (char*) calloc(40, sizeof(char)); 
							memset(hash_ChunkRegen,'\0',sizeof(hash_ChunkRegen));//initialize hash_ChunkRegen with null values.
							hash_ChunkOriginal = (char*) calloc(40, sizeof(char)); 
							memset(hash_ChunkOriginal,'\0',sizeof(hash_ChunkOriginal));
							//calCheckSum function calculates hash checksum value for 'chunk' file and store in hash_ChunkRegen.
							calChecksum(chunk,hash_ChunkRegen);
							
							// hash_ChunkOriginal stores the original hash checksum for 'chunk' file.
				        		(i==0)?(hash_ChunkOriginal=d.hash_folder0):(hash_ChunkOriginal=hash_ChunkOriginal);
				        		(i==1)?(hash_ChunkOriginal=d.hash_folder1):(hash_ChunkOriginal=hash_ChunkOriginal);
				        		(i==2)?(hash_ChunkOriginal=d.hash_folder2):(hash_ChunkOriginal=hash_ChunkOriginal);
				        		(i==3)?(hash_ChunkOriginal=d.hash_folder3):(hash_ChunkOriginal=hash_ChunkOriginal);
				        		(i==4)?(hash_ChunkOriginal=d.hash_folder4):(hash_ChunkOriginal=hash_ChunkOriginal);
				        		(i==5)?(hash_ChunkOriginal=d.hash_folder5):(hash_ChunkOriginal=hash_ChunkOriginal);
				        		(i==6)?(hash_ChunkOriginal=d.hash_folder6):(hash_ChunkOriginal=hash_ChunkOriginal);
				        		(i==7)?(hash_ChunkOriginal=d.hash_folder7):(hash_ChunkOriginal=hash_ChunkOriginal);
								//populate the 'availableChunks' array.
								//This check is needed to verify that the chunk was not changed after the 'put' operation.
				        		if(strcmp(hash_ChunkRegen,hash_ChunkOriginal)==0){
				        			availableChunks[i] = 1 ;
				        		}	
				        		else{
				        		//chunk hashes not equal
				        			availableChunks[i]=0 ;
				        		}
							// closing the open files and deallocating pointers to files.
							close(fch);
							fclose(fchunk); 
							fchunk = NULL ;
						}
					}
					
					for(i=0 ; i<NUM_PARITY ; ++i)
					{	//storing parity file name in 'chunk' variable. 
						sprintf(chunk, DIRECTORY_PATH"parity%d/%d_parity%d.%s%c", i, uid, i, file_extension, '\0');
						fchunk = fopen(chunk, "r") ;// open the parity file in read mode

						if(fchunk == NULL)// if parity file is not found
						{
							availableChunks[i+NUM_DATA] = 0;
						}
						else
						{
							//memory allocation of 40 bytes for storing hash checksum value for chunk parity file.
							hash_ChunkRegen = (char*) calloc(40, sizeof(char)); 
							//initialize hash_ChunkRegen with null values.
							memset(hash_ChunkRegen,'\0',sizeof(hash_ChunkRegen));
							hash_ChunkOriginal = (char*) calloc(40, sizeof(char)); 
							memset(hash_ChunkOriginal,'\0',sizeof(hash_ChunkOriginal));
							//calCheckSum function calculates hash checksum value for 'chunk' parity file and store in hash_ChunkRegen.
							calChecksum(chunk,hash_ChunkRegen);
							
							// hash_ChunkOriginal stores the original hash checksum for 'chunk' file.
								(i==0)?(hash_ChunkOriginal=d.hash_parity0):"";
				        		(i==1)?(hash_ChunkOriginal=d.hash_parity1):"";
				        		(i==2)?(hash_ChunkOriginal=d.hash_parity2):"";
				        		//This check is needed to verify that the parity chunk was not changed after the 'put' operation.
				        		if(strcmp(hash_ChunkRegen,hash_ChunkOriginal)==0){
				        			availableChunks[i+NUM_DATA] = 1;
				        		}
				        		else{// hash checksum is not equal means the parity chunk is changed.
				        		    availableChunks[i+NUM_DATA] = 0;
				   	    		}
							fclose(fchunk);
							fchunk = NULL ;		
						}
					}
					// closing the open files and deallocating pointers to files.
					close(fd);
					fclose(ft) ;
					ft = NULL ;
				}
				
				//available_buffs allocates memory for data chunks
				for(i=0; i<NUM_DATA; i++)
				{
					available_buffs[i]=(unsigned char *)calloc(chunk_size, sizeof(unsigned char));
				}
				
				// Code to populate the available_buffs 
				// code to check the availability of chunks and parity chunks.
				j = 0;// iterator for available_buffs
				for(i=0 ; i<NUM_DATA ; ++i)
				{
					if(availableChunks[i] == 1)// if chunk is present
					{	
						//opening the parity chunk file in read mode.
						sprintf(chunk, DIRECTORY_PATH"folder%d/%d_chunk%d.%s%c", i, uid, i, file_extension, '\0');	
						fchunk = fopen(chunk, "r") ;
						if(fchunk == NULL)
						{
							printf("\n\tData file could not be opened");	
						}
						else
						{	//copying chunk data into availabe_buffs
							fch = open(chunk, O_RDONLY | O_CREAT);
							read(fch, buff, chunk_size);
							memcpy(available_buffs[j], buff, chunk_size);
							j++;

							close(fch);
							fclose(fchunk);
							fchunk = NULL;
						}		
					}			
				}
				
				for(i=0 ; i<NUM_PARITY ; ++i)
				{
					if(availableChunks[i + NUM_DATA] == 1)// if parity chunk is present
					{
						//opening the parity chunk file in read mode.
						sprintf(chunk, DIRECTORY_PATH"parity%d/%d_parity%d.%s%c", i, uid, i, file_extension, '\0');	
						fchunk = fopen(chunk, "r") ;
						if(fchunk == NULL)
						{
							printf("\n\tData file could not be opened");	
						}
						else
						{	//copying chunk data into availabe_buffs
							fch = open(chunk, O_RDONLY| O_CREAT);
							read(fch, buff, chunk_size);
							if(j < NUM_DATA)
							{ 
								memcpy(available_buffs[j], buff, chunk_size);
								j++;
							}

							close(fch);
							fclose(fchunk);
							fchunk = NULL;	
						}		
					}			
				}
				
				
				//calclating missing chunks
				for(i = 0; i < NUM_DATA; ++i)
				{
					if(availableChunks[i] == 0)
					{
						printf("\n\tChunk %d is missing.", i);
						missingChunks++;
					}
				}
				//calculating missing parities
				for(i = 0 ; i < NUM_PARITY ; ++i)
				{
					if(availableChunks[i + NUM_DATA] == 0)
					{
						printf("\n\tParity %d is missing.", i);
						missingParity++;
					}
				}
				//Number of missing chunks should be less than number of parity used.	
				// if number of missing chunks is greater than number of parity then the file cannot be regenerated. 
				if((missingChunks + missingParity)>NUM_PARITY)
				{
					printf("\n\n\tNo. of missing files exceeds %d, hence cannot retrieve file. ", NUM_PARITY);
					succ = -1 ;
					return succ;
				}
				else{
					//filling marr array with the lost chunk file numbers.
					// **** marr only contains the index of missing chunk file. It does not contain the index of missing parity files.
					for(i = 0, j = 0 ; i < NUM_DATA && j < NUM_PARITY ; ++i)
					{
						if(availableChunks[i] == 0)
						{
							marr[j] = i ;
							j++;
						}
					}
					// Regeneration missing chunks if any
					if(missingChunks > 0)
					{
						printf("\n\n\tAvailable Chunks : ");
						for(i = 0 ; i < (NUM_DATA + NUM_PARITY) ; ++i)
						{
							printf("%d  ", availableChunks[i]);
						}
						//gf_gen_rs_matrix ???????
						gf_gen_rs_matrix(gen, (NUM_DATA + NUM_PARITY), NUM_DATA);
						
						// Printing generator matrix
						printf("\n\n\tGenerator Matrix : ");
						for(i = 0, j = 0 ; i < (NUM_DATA + NUM_PARITY) * NUM_DATA ; ++i)
						{
							if(i % NUM_DATA == 0)
							{
								printf("\n\t");
							}
							printf("%d \t", gen[i]);
						}
						// Populating Recovery Matrix
						for(i = 0, k = 0 ; i < (NUM_DATA + NUM_PARITY) ; ++i)
						{
							if(availableChunks[i] == 1)
							{
								for(j = 0 ; j < NUM_DATA ; ++j)
								{
									recovery[(k * NUM_DATA) + j] = gen[(i * NUM_DATA) + j];
								}
								k++;
							}
						}
						// ****  while loop won't be needed because in the edge testcase where the number of missing bits is equal to
						// number of parity bits, the value of k=NUM_DATA.   *****
						i = NUM_DATA;
						while(k < NUM_DATA)
						{
							for(j = 0 ; j < NUM_DATA ; ++j)
							{
								recovery[(k * NUM_DATA) + j] = gen[(i * NUM_DATA) + j];
							}
							k++;
							i++;
						}
						// printing Recovery Matrix
						printf("\n\n\tRecovery Matrix : ");
						for(i = 0 ; i < (NUM_DATA) * NUM_DATA ; ++i)
						{
							if(i % NUM_DATA == 0)
							{
								printf("\n\t");
							}
							printf("%d \t", recovery[i]);
						}
						
						gf_invert_matrix(recovery, decode, NUM_DATA);

						//Decode matrix is inverse of recovery matrix
						//Printing Decode Matrix
						// **** In the for loop no need to use variable j *****
						printf("\n\n\tDecode Matrix (Inverse of recovery matrix) : ");
						for(i = 0, j = 0 ; i < (NUM_DATA * NUM_DATA) && (j < NUM_DATA * NUM_DATA) ; ++i)
						{
							if(i % NUM_DATA == 0)
							{
								printf("\n\t");
							}
							printf("%d \t", decode[i]);
						}

						printf("\n\n\tFailed row decode matrix : \n\t");
						
						// frdm contains missing chunk's rows of inverted matrix 
						// printing the missing chunk's decode rows
						for(i = 0, k = 0; i < missingChunks; i++)
						{	//**** inner for loop should be inside if condition *****
							for(j = 0 ; j < NUM_DATA ; j++)  
							{	//marr array contains the index of missing chunks.
								if(marr[i] != -1)// 
								{
									frdm[k] = decode[(marr[i] * NUM_DATA) + j];
									printf("%d ", frdm[k]);
									k++;
								}
							}
							printf("\n\t");
						} 
						// ec_init_tables ????
						ec_init_tables(NUM_DATA, NUM_PARITY, frdm, r_tables);

						// allocating memory for parity chunks
						for(i = 0 ; i < (NUM_PARITY) ; ++i)
						{
							temp_buffs[i] = (unsigned char *)calloc(chunk_size, sizeof(unsigned char));
						}

						// ec_encode_data_base ????
						ec_encode_data_base(file_size / NUM_DATA, NUM_DATA, missingChunks, r_tables, available_buffs, temp_buffs);  

						// Printing index of regenerated missing chunks
						printf("\n\tRegenerated missing chunks : ");
						for(i = 0, j = 0; i < NUM_DATA ; i++)
						{
							if(availableChunks[i] != 1)
							{
								printf("  %d", i);
								marr[j] = i;   // skip
								j++;
							}
						}
						j--;

						// Regenerating all the missing chunks from the temp_buffs buffer
						for(i=0, j=0 ; i<NUM_DATA ; ++i)
						{
							if(availableChunks[i] == 0)
							{
								sprintf(chunk, DIRECTORY_PATH"folder%d/%d_chunk%d.%s%c", i, uid, i, file_extension, '\0');			
								fchunk=fopen(chunk,"w");
						    		
								if(fchunk == NULL)
								{
									printf("\n\tMissing data file %d could not be created!", i);
								}
								else
								{	// copying data from temop_buffs into new created file
									fch = open(chunk, O_RDWR | O_CREAT);
									write(fch, temp_buffs[j], chunk_size);
									j++;

									//closing the open files and deallocating pointers
									close(fch);
									fclose(fchunk);
									fchunk = NULL ;
								}
							}
						}

						/*printf("\n\t Temp buffs : ");
						for(i = 0 ; i < missingChunks ; i++)
						{
							printf("\n\t Temp_buff[%d] : %s", i, temp_buffs[i]);
						}*/

						flag2 = 1 ; //flag2 determines whether there are any missing chunks in the file.
						strcpy(targetFilePath, targetPath_old);	
						succ = get(uid,targetFilePath,uid_f);

					}
					// Regenerating missing parity blocks if any
					if(flag2 != 1 && missingParity > 0)
					{
						//memory allocation for parity
						for(i=0; i<NUM_PARITY; i++)
						{
							paritybuffs[i] = (unsigned char *)calloc(chunk_size, sizeof(unsigned char));
						}

						//????
						gf_gen_rs_matrix(gen, (NUM_DATA + NUM_PARITY), NUM_DATA);
						ec_init_tables(NUM_DATA, NUM_PARITY, &gen[NUM_DATA * NUM_DATA], g_tbls);
						ec_encode_data_base(file_size / NUM_DATA, NUM_DATA, NUM_PARITY, g_tbls, available_buffs, paritybuffs);

						printf("\n\n\tParity Chunks Regenerated \n");

						/*for(i = 0 ; i < missingParity ; i++)
						{
							printf("\n\tParity Chunk %d : %s", i, paritybuffs[i]);
						}*/

						//copying data in paritybuffs to parity files
						for(i=0; i<NUM_PARITY; i++)
						{	//opening the parity chunk folder in read mode.
					    	sprintf(directory, DIRECTORY_PATH"parity%d%c", i, '\0');
							dirSucc = isDir(directory);
							if(dirSucc != 0)
							{
								check = mkdir(directory, 0777); 
							}
							if(check != 0)// Parity folder creation failed
							{
								printf("\n\t Parity Folder creation failed!!!");
								succ = -1;
							}
							// copying parity file name in chunk.
							sprintf(chunk, DIRECTORY_PATH"parity%d/%d_parity%d.%s%c", i, uid, i, file_extension, '\0');
							fchunk=fopen(chunk,"w");
					    		
							if(fchunk == NULL)
							{
								printf("\n\tParity file could not be created!");
							}
							else
							{	//copying data from parity_buffs to newly created parity chunk.
								fch = open(chunk, O_RDWR | O_CREAT);
								write(fch, paritybuffs[i], chunk_size);
								// closing the file and deallocating pointers to parity chunk.
								close(fch);
								fclose(fchunk);
								fchunk = NULL;
							}
					    	}
					}
				}	
			}	
			//directory TargetPath is incorrect
			else
			{
				printf("\n\tTarget File path : %s", targetFilePath);
				printf("\n\n\tThe given target path is incorrect");
				printf("\n\tPlease enter correct directory path : ");
				memset(targetFilePath,'\0',sizeof(targetFilePath));
				//user to renter the target path
				scanf("%s",targetFilePath);
				strcpy(targetPath_old, targetFilePath);	
				flag=1;
			}
		}while(flag != 0);
		
	}
    return succ;//success code
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

//Put Function
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
    	//char int_str[20]={'\0'}; int_str is unused variable
    	int i=0; // pointer to read file_path
    	// char ch='\0';
    	// int k=0;
    	// int j=0;
    	int uid=-1;// storing unique ID for a datafile
    	int rs=0;
	int fd=0;// fd - file descriptor
	int fch=0;
	char file_extension[10]={'\0'};
	char directory[100] = {'\0'};
	int dirSucc = 0;
	int check=0;
	char *hash = NULL ;
	char *hashChunks=NULL;;
	struct uid_filemap a;
	
	// fp is a file pointing at location 'file_path' 	
	fp = fopen(file_path, "r");
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

	        //fd = open(file_path, O_RDONLY | O_CREAT);
		
		// checking if file size(in bytes) is multiple of 8
		if((file_size % NUM_DATA) != 0)
		{
			printf("\n\tSystem Requirement : Filesize should be multiple of %d!", NUM_DATA);
			printf("\n\tCurrent filesize : %d bytes\n", file_size);
		}
		else
		{       //getFileName - Function to retrieve a file name from a file path.
			getFileName(file_path, file_name); 
			//getFileExtension - Function to retrieve file_extension from a file path.
			getFileExtension(file_name, file_extension);
			//generateFileUid - Function to retrieve unique Id for a file.
			uid = generateFileUid(file_path);
                        
                        // Populating structure 'uid_filemap' into a structure variable 'a'.  
                  	a.uid=uid;
			strcpy(a.filename ,file_name);
			strcpy(a.filepath,file_path);
			a.size = file_size;
			//allocating memory to a character array of size 40bytes string and initialising to null value
			hash = (char*) calloc(40, sizeof(char));
			//calCheckSum is used to calculate hash checksum value of a file and store it in a 'hash' variable.
		    	calChecksum(file_path, hash); 
			strcpy(a.hash_OriginalFile , hash);

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
				{       // creating a folder along the 'directory' path.
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

			//copying data in paritybuffs to parity files
			for(i=0; i<NUM_PARITY; i++)
			{       //creating directory for parity chunks. 
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
				{       // write the parity chunks from the 'paritybuffs'.
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
		        //closing files.
			close(fd);
			fclose(fp);
			fp = NULL;
		}
	}
	// return the uid [Fail=-1]
    	return uid;
    	
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
	
	display(arr[bucket_no]);

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

void display(struct Node *temp)
{
	printf("\n\tHash Table records : ");

	while(temp!=NULL)
	{
		printf("\n\tuid = %d ",temp->uid);
		printf("\n\tFilepath = %s ",temp->filepath);
		printf("\n\tFile Size = %d bytes",temp->size);
		printf("\n\tChecksum = %s\n", temp->hash);

		temp=temp->next;
	}
}

void list(struct Node **arr)
{
	int i=0;
	struct Node *temp=NULL;

	for(i=0; i<BUCKETSIZE; i++)
	{
		printf("\n\tBucket Number : %d",i);

		if(arr[i]==NULL)
		{
			printf("\n\tBucket Empty\n");
		}
		else
		{
			temp=arr[i];
			display(temp);
		}
	}
}

void str_reverse(char fstr[50])
{
    int i=0 ;
    int j=0 ;
    int count=0 ;
    char fstr2[50];
    
    count = strlen(fstr) ;
    for(i=count-1, j=0 ; i>=0 ; i--, j++)
    {
        fstr2[j] = fstr[i] ;
        fstr2[j+1] = '\0' ;
    } 
    
    strcpy(fstr, fstr2);
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
int addARecord_uid(struct uid_filemap s)
{

	
	int flag=0;//to return success
	int rs=0;
	FILE *fout=NULL;//file pointer
	FILE *fin = NULL;
	struct uid_filemap x ;
	//opening the file in rb+ mode
	fout=fopen("/home/yashgajalwar/Desktop/erasure/uid_file.txt","ab+");
	fin=fopen("/home/yashgajalwar/Desktop/erasure/uid_file.txt","rb");
	
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


				
				
	   	     }

	   	}while(rs==1);
		fclose(fin);
		fin=NULL;
		
	}


	return flag;


}

int search(char * filename, struct uid_filemap * res)
{
 	int rs=0;
    	FILE *fin=NULL;//file pointer
	struct uid_filemap x={0};//to search a record
    	int temp=0;//temporary variable
    	int flag =0;
    	//opening the file in rb mode
	fin=fopen("/home/yashgajalwar/Desktop/erasure/uid_file.txt","rb");
	
	if(fin==NULL)
	{
	    flag=-1;//file not opened
	}
	
	else
	{
		 do
	   	 {

			
	   	     rs=fread(&x,sizeof(struct uid_filemap),1,fin);//reading from file
	   	     if(rs==1 && strcmp(x.filename,filename)==0)
	   	     {
	   			 
	   		
				res->uid = x.uid;
				res->size = x.size;
				strcpy(res->filepath,x.filepath);
				strcpy(res->hash_OriginalFile , x.hash_OriginalFile);
				strcpy(res->hash_folder0 , x.hash_folder0);
				strcpy(res->hash_folder1 , x.hash_folder1);
				strcpy(res->hash_folder2 , x.hash_folder2);
				strcpy(res->hash_folder3 , x.hash_folder3);
				strcpy(res->hash_folder4 , x.hash_folder4);
				strcpy(res->hash_folder5 , x.hash_folder5);
				strcpy(res->hash_folder6 , x.hash_folder6);
				strcpy(res->hash_folder7 , x.hash_folder7);
				strcpy(res->hash_parity0 , x.hash_parity0);
				strcpy(res->hash_parity1 , x.hash_parity1);
				strcpy(res->hash_parity2 , x.hash_parity2);
				
	   	     }

	   	}while(rs==1);
	    
	    	fclose(fin);
	    	fin=NULL;
	
	}
	return flag;
	
	
}

int search_uid(int uid, struct uid_filemap * res)
{
 	int rs=0;
    	FILE *fin=NULL;//file pointer
	struct uid_filemap x={0};//to search a record
    	int temp=0;//temporary variable
    	int flag =0;
    	//opening the file in rb mode
	fin=fopen("/home/yashgajalwar/Desktop/erasure/uid_file.txt","rb");
	
	if(fin==NULLc)
	{
	    flag=-1;//file not opened
	}
	
	else
	{
		  flag = 0;
		 do
	   	 {

			
	   	     rs=fread(&x,sizeof(struct uid_filemap),1,fin);//reading from file
	   	     if(rs==1 && x.uid==uid)
	   	     {
	   			 
	   		
				strcpy(res->filename,x.filename);
				res->size = x.size;
				strcpy(res->filepath,x.filepath);
				strcpy(res->hash_OriginalFile , x.hash_OriginalFile);
				strcpy(res->hash_folder0 , x.hash_folder0);
				strcpy(res->hash_folder1 , x.hash_folder1);
				strcpy(res->hash_folder2 , x.hash_folder2);
				strcpy(res->hash_folder3 , x.hash_folder3);
				strcpy(res->hash_folder4 , x.hash_folder4);
				strcpy(res->hash_folder5 , x.hash_folder5);
				strcpy(res->hash_folder6 , x.hash_folder6);
				strcpy(res->hash_folder7 , x.hash_folder7);
				strcpy(res->hash_parity0 , x.hash_parity0);
				strcpy(res->hash_parity1 , x.hash_parity1);
				strcpy(res->hash_parity2 , x.hash_parity2);
				
				
	   	     }

	   	}while(rs==1);
	    
	    	fclose(fin);
	    	fin=NULL;
	
	}
	return flag;
	
	
}