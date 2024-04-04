#include "/home/yashgajalwar/Desktop/erasure/march/include/get.h"
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

// int count =0;
// struct Map myMap;
// struct Node **arr;
// unsigned char gen[(NUM_DATA + NUM_PARITY) * (NUM_DATA + NUM_PARITY)]; //Generator matrix consist of identity matrix and parity matrix - gen[((8+3),3)]
// unsigned char g_tbls[(NUM_DATA + NUM_PARITY) * (NUM_DATA + NUM_PARITY) * 32]; //g_tbls is internal structure used in ISA-L. Same as parity matrix.
// unsigned char *databuffs; //databufs to be allocated using calloc as per file size
// unsigned char *paritybuffs[NUM_PARITY];	//parities calculated will be stored here
// unsigned char *datachunks[NUM_DATA]; //data from the chunk files will be stored here

int get(int uid,char *targetFilePath,char * uid_f,int *container_index,unsigned char **container)
{	
    count++;
	printf("\n\n Executing Get Function Round No: %d\n",count);
    if(count>3){
        printf("\nGet function executing recursively\n");
        return -1;
    }

	FILE *fchunk=NULL;
	FILE *ft = NULL;	
	int succ=0;
	char cc = '\0';
	char *newFilePath = NULL;
	char chunk[100] = {'\0'};
	int flag=0;
	int flag2=0;
	int missingChunks=0;
	int missingParity=0;
	int i=0;
	int j=0;
	int k=0;
	int r=0;
	char file_name[50]={'\0'};	
	char file_path[50]={'\0'};
	char int_str[20]={'\0'};
	char targetPath_old[50] = {'\0'};
	int fch = 0;
	int fd = 0;
	int chunk_size = 0;
	int file_size = 0;
	char directory[100] = {'\0'};
	int dirSucc = 0;
	int check=0;
	char file_extension[10]={'\0'};					
	int availableChunks[NUM_DATA + NUM_PARITY] = {0};
	unsigned char recovery[NUM_DATA * NUM_DATA];
	unsigned char decode[NUM_DATA * NUM_DATA];
	unsigned char frdm[NUM_DATA * NUM_PARITY];
	unsigned char r_tables[(NUM_DATA + NUM_PARITY) * (NUM_DATA + NUM_PARITY) * 32];
	unsigned char *temp_buffs[NUM_PARITY] = {NULL};
	unsigned char *available_buffs[NUM_DATA] = {NULL};
	unsigned char *buff = NULL;
	int marr[NUM_PARITY] = {0};
	char *hash_OriginalFile = NULL;
	char *hash_RegenFile = NULL;
	char *hash_ChunkOriginal = NULL;
	char *hash_ChunkRegen = NULL;
	struct Node *temp = arr[uid % BUCKETSIZE];	

	for(i = 0 ; i < NUM_PARITY ; ++i)
	{
		marr[i] = -1 ;
	}	
	struct uid_filemap d;

	flag = search_Db(uid,&d);
	// flag = search(uid_f,&d);
	file_size = d.size;
	strcpy(file_path , d.filepath);
	hash_OriginalFile = d.hash_OriginalFile;
	int padding=d.padding;

	if(flag == -1)
	{
		printf("\n\tFile with unique ID %d does not exist in the system!", uid);
		succ = -1 ;	
	}
	else if((file_size % NUM_DATA) != 0)
	{
		printf("\n\tSystem Requirement : Filesize should be multiple of %d!", NUM_DATA);
		printf("\n\tCurrent filesize : %d bytes \n", file_size);
		succ = -1 ;
	}
	else
	{
		flag = 0;	//reset flag
		for(i=0; file_path[i] != '\0'; i++)
		{
			if(file_path[i] == '/')
			{
				flag2=1;
			}	
		}
		
		if(flag2 == 0)
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
		flag2 = 0 ;	// to regenerate parity if missing
		chunk_size = file_size / NUM_DATA ;
		buff=(unsigned char *)calloc(chunk_size + NUM_DATA, sizeof(unsigned char));
		strcpy(targetPath_old, targetFilePath);	

		do
		{
			if(isDir(targetFilePath)==0)
			{
				flag=0;	
				strcat(targetFilePath, file_name);
				sprintf(int_str,"%d%c", uid, '\0');	
				ft = fopen(targetFilePath,"a");//open the target file

				if(ft == NULL)
				{   
					printf("\n\tTarget file could not be opened!!");
					succ = -1;
				}
				else
				{
					fd = open(targetFilePath, O_RDWR | O_CREAT);
					for(i = 0; i < (NUM_DATA+NUM_PARITY) ; ++i)
					{   
						if(i<NUM_DATA){
                            sprintf(directory, DIRECTORY_PATH"folder%d%c", i, '\0');
                        }
                        else{
                            sprintf(directory, DIRECTORY_PATH"parity%d%c", (i-8), '\0');
                        }

						dirSucc = isDir(directory);
						if(dirSucc != 0)
						{
							check = mkdir(directory, 0777); 
						}
						if(check != 0)
						{   
							printf("\n\t Folder creation failed!!!");
							succ = -2;
						}

                        if(i<NUM_DATA){
                            sprintf(chunk, DIRECTORY_PATH"folder%d/%d_chunk%d.%s%c", i, uid, i, file_extension, '\0');
                        }
                        else{
                            sprintf(chunk, DIRECTORY_PATH"parity%d/%d_parity%d.%s%c", (i-8), uid, (i-8), file_extension, '\0');
                        }
						fchunk = fopen(chunk, "r");
						if(fchunk == NULL)
						{   
                            // have to check checksum values if available chunk is regenerated
							if(container_index[i]==1)
							{
								unsigned char *data_from_map = (unsigned char *)calloc(chunk_size, sizeof(unsigned char));
								//searchMap(&myMap,i,data_from_map,chunk_size);
								for(int itr=0;itr<myMap.size;itr++){
									if(myMap.data[itr].key==i)
									{
										printf("in Search function:\t");
										data_from_map = myMap.data[itr].value;
										memcpy(data_from_map, myMap.data[itr].value, chunk_size);
									}
								}
								if(i==(NUM_DATA-1))
								{
									write(fd,data_from_map,chunk_size-padding);
								}
								else if(i<NUM_DATA)
								{
									write(fd,data_from_map,chunk_size);		
								}
								availableChunks[i]=1;

							}
							else
							{ 
								availableChunks[i]=0;
							}
                        }
						else
						{   
							hash_ChunkRegen = (char*) calloc(40, sizeof(char)); 
							memset(hash_ChunkRegen,'\0',sizeof(hash_ChunkRegen));
							hash_ChunkOriginal = (char*) calloc(40, sizeof(char)); 
							memset(hash_ChunkOriginal,'\0',sizeof(hash_ChunkOriginal));
							calChecksum(chunk,hash_ChunkRegen);
							
                            (i==0)?(hash_ChunkOriginal=d.hash_folder0):(hash_ChunkOriginal=hash_ChunkOriginal);
                            (i==1)?(hash_ChunkOriginal=d.hash_folder1):(hash_ChunkOriginal=hash_ChunkOriginal);
                            (i==2)?(hash_ChunkOriginal=d.hash_folder2):(hash_ChunkOriginal=hash_ChunkOriginal);
                            (i==3)?(hash_ChunkOriginal=d.hash_folder3):(hash_ChunkOriginal=hash_ChunkOriginal);
                            (i==4)?(hash_ChunkOriginal=d.hash_folder4):(hash_ChunkOriginal=hash_ChunkOriginal);
                            (i==5)?(hash_ChunkOriginal=d.hash_folder5):(hash_ChunkOriginal=hash_ChunkOriginal);
                            (i==6)?(hash_ChunkOriginal=d.hash_folder6):(hash_ChunkOriginal=hash_ChunkOriginal);
                            (i==7)?(hash_ChunkOriginal=d.hash_folder7):(hash_ChunkOriginal=hash_ChunkOriginal);
                            (i==8)?(hash_ChunkOriginal=d.hash_parity0):"";
                            (i==9)?(hash_ChunkOriginal=d.hash_parity1):"";
                            (i==10)?(hash_ChunkOriginal=d.hash_parity2):"";
                            if(strcmp(hash_ChunkRegen,hash_ChunkOriginal)==0)
							{
                                availableChunks[i] = 1 ;
								if(i==(NUM_DATA-1))
								{
									printf("Deleting padding: %d bytes\n\n",padding);
									fch = open(chunk, O_RDONLY | O_CREAT);
                                    read(fch, buff, chunk_size-padding);
                                    write(fd, buff, chunk_size-padding);
								}
                                else if(i<NUM_DATA)
								{
                                    fch = open(chunk, O_RDONLY | O_CREAT);
                                    read(fch, buff, chunk_size);
                                    write(fd, buff, chunk_size);
                                }
                                close(fch); 
                            }	
                            else
							{
                                printf("Chunk values are not equal\n");
                                availableChunks[i]=0 ;   
                            }
							fclose(fchunk); 
							fchunk = NULL ;
						}
					}
					close(fd);
					fclose(ft) ;
					ft = NULL ;
				}
				
				//available buffs memory allocation for data chunks
				for(i=0; i<NUM_DATA; i++)
				{
					available_buffs[i]=(unsigned char *)calloc(chunk_size, sizeof(unsigned char));
				}
				
				//Code to populate the available_buffs 
				j = 0;
				for(i=0 ; i<(NUM_DATA+NUM_PARITY) ; ++i)
				{
					if(availableChunks[i] == 1)
					{
						if(i<NUM_DATA){
                            sprintf(chunk, DIRECTORY_PATH"folder%d/%d_chunk%d.%s%c", i, uid, i, file_extension, '\0');
                        }
                        else{
                            sprintf(chunk, DIRECTORY_PATH"parity%d/%d_parity%d.%s%c", (i-8), uid, (i-8), file_extension, '\0');	
                        }	
						fchunk = fopen(chunk, "r") ;
						if(fchunk == NULL)
						{
							printf("\n\tData file could not be opened");	
						}
						else
						{
							fch = open(chunk, O_RDONLY | O_CREAT);
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
				for(i = 0; i < (NUM_DATA+NUM_PARITY); ++i)
				{
					if(availableChunks[i] == 0)
					{   if(i<NUM_DATA){
                            printf("\n\tChunk %d is missing.", i);
						    missingChunks++;
                        }
                        else{
                            printf("\n\tParity %d is missing.", i);
						    missingParity++;
                        }
						
					}
				}
				if((missingChunks + missingParity)>NUM_PARITY)
				{
					printf("\n\n\tNo. of missing files exceeds %d, hence cannot retrieve file. ", NUM_PARITY);
					succ = -1 ;
					return succ;
				}
				else{
					//filling marr
					for(i = 0, j = 0 ; i < NUM_DATA && j < NUM_PARITY ; ++i)
					{
						if(availableChunks[i] == 0)
						{
							marr[j] = i ;
							j++;
						}
					}
					// Regeneration Code
					if(missingChunks > 0)
					{
						printf("\n\n\tAvailable Chunks : ");
						for(i = 0 ; i < (NUM_DATA + NUM_PARITY) ; ++i)
						{
							printf("%d  ", availableChunks[i]);
						}
					
						gf_gen_rs_matrix(gen, (NUM_DATA + NUM_PARITY), NUM_DATA);
						
						printf("\n\n\tGenerator Matrix : ");
						for(i = 0, j = 0 ; i < (NUM_DATA + NUM_PARITY) * NUM_DATA ; ++i)
						{
							if(i % NUM_DATA == 0)
							{
								printf("\n\t");
							}
							printf("%d \t", gen[i]);
						}

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
						for(i = 0, k = 0; i < missingChunks; i++)
						{
							for(j = 0 ; j < NUM_DATA ; j++)  
							{
								if(marr[i] != -1)
								{
									frdm[k] = decode[(marr[i] * NUM_DATA) + j];
									printf("%d ", frdm[k]);
									k++;
								}
							}
							printf("\n\t");
						} 
					
						ec_init_tables(NUM_DATA, NUM_PARITY, frdm, r_tables);

						for(i = 0 ; i < (NUM_PARITY) ; ++i)
						{
							temp_buffs[i] = (unsigned char *)calloc(chunk_size, sizeof(unsigned char));
							
						}

						ec_encode_data_base(file_size / NUM_DATA, NUM_DATA, missingChunks, r_tables, available_buffs, temp_buffs);  
						printf("\n\tRegenerated missing chunks : ");
						for(i = 0, j = 0; i < NUM_DATA ; i++)
						{
							if(availableChunks[i] != 1)
							{
								printf("  %d", i);
								marr[j] = i;
								j++;
							}
						}
						j--;

						for(i=0, j=0 ; i<NUM_DATA ; ++i)
						{
							if(availableChunks[i] == 0)
							{
								sprintf(chunk, DIRECTORY_PATH"folder%d/%d_chunk%d.%s%c", i, uid, i, file_extension, '\0');			
								fchunk=fopen(chunk,"w");	
								if(fchunk == NULL)
								{	
									insert(&myMap,i,temp_buffs[j],chunk_size);
									printf("\n\tMissing data file %d could not be created! \n", i);
									container_index[i]=1;
									j++;
								}
								else
								{
									fch = open(chunk, O_RDWR | O_CREAT);
									write(fch, temp_buffs[j], chunk_size);
									j++;

									close(fch);
									fclose(fchunk);
									fchunk = NULL ;
								}
							}
						}
						flag2 = 1 ;
						strcpy(targetFilePath, targetPath_old);	
						succ = get(uid,targetFilePath,uid_f,container_index,container);
                        // printf("Returning from get %d",count);
                        count--;
					}
					if(flag2 != 1 && missingParity > 0)
					{
						//memory allocation for parity
						for(i=0; i<NUM_PARITY; i++)
						{
							paritybuffs[i] = (unsigned char *)calloc(chunk_size, sizeof(unsigned char));
						}
						
						gf_gen_rs_matrix(gen, (NUM_DATA + NUM_PARITY), NUM_DATA);
						ec_init_tables(NUM_DATA, NUM_PARITY, &gen[NUM_DATA * NUM_DATA], g_tbls);
						ec_encode_data_base(file_size / NUM_DATA, NUM_DATA, NUM_PARITY, g_tbls, available_buffs, paritybuffs);

						printf("\n\n\tParity Chunks Regenerated \n");
						//copying data in paritybuffs to parity files
						for(i=0; i<NUM_PARITY; i++)
						{
					    	sprintf(directory, DIRECTORY_PATH"parity%d%c", i, '\0');
							dirSucc = isDir(directory);
							if(dirSucc != 0)
							{
								check = mkdir(directory, 0777); 
							}
							if(check != 0)
							{
								printf("\n\t Parity Folder creation failed!!!");
								succ = -2;
							}
							sprintf(chunk, DIRECTORY_PATH"parity%d/%d_parity%d.%s%c", i, uid, i, file_extension, '\0');
							fchunk=fopen(chunk,"w");
							if(fchunk == NULL)
							{
								printf("\n\tParity file could not be created!");
								insert(&myMap,i+8,paritybuffs[i],chunk_size);
								container_index[i+8]=1;
							}
							else
							{
								fch = open(chunk, O_RDWR | O_CREAT);
								write(fch, paritybuffs[i], chunk_size);
								close(fch);
								fclose(fchunk);
								fchunk = NULL;
							}
						}
					}
				}	
			}	
			//directory filepath
			else
			{
				printf("\n\tTarget File path : %s", targetFilePath);
				printf("\n\n\tThe given target path is incorrect");
				printf("\n\tPlease enter correct directory path : ");
				memset(targetFilePath,'\0',sizeof(targetFilePath));
				scanf("%s",targetFilePath);
				strcpy(targetPath_old, targetFilePath);	
				flag=1;
			}
		}while(flag != 0);	
	}
	return succ;//success code
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
				res->padding = x.padding;
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
	if(fin==NULL)
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



int search_Db(int uid,struct uid_filemap * d){
	printf("In search_Db function\n\n");
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
    sprintf(query, "SELECT * FROM tb1 WHERE uid = %d",uid);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "Failed to execute query: Error: %s\n", mysql_error(conn));
        mysql_close(conn);
        return 1;
    }
	res = mysql_store_result(conn);

    if (res == NULL) {
        fprintf(stderr, "No data returned.\n");
        mysql_close(conn);
        return 1;
    }

    if ((row = mysql_fetch_row(res))) {
        sscanf(row[0], "%d", &d->uid);
        sscanf(row[1], "%s", d->filename);
		sscanf(row[2], "%ld", &d->size);
		sscanf(row[3], "%s", d->filepath);
		sscanf(row[4], "%s", d->hash_OriginalFile);
		sscanf(row[5], "%s", d->hash_folder0);
		sscanf(row[6], "%s", d->hash_folder1);
		sscanf(row[7], "%s", d->hash_folder2);
		sscanf(row[8], "%s", d->hash_folder3);
		sscanf(row[9], "%s", d->hash_folder4);
		sscanf(row[10], "%s", d->hash_folder5);
		sscanf(row[11], "%s", d->hash_folder6);
		sscanf(row[12], "%s", d->hash_folder7);
		sscanf(row[13], "%s", d->hash_parity0);
		sscanf(row[14], "%s", d->hash_parity1);
		sscanf(row[15], "%s", d->hash_parity2);
		sscanf(row[16], "%d", &d->padding);
		 
        printf("ID: %d\n", d->uid);
        printf("Name: %s\n", d->filename);
        
    } else {
        printf("Record with ID %d not found.\n", uid);
		return -1;
    }

    mysql_free_result(res);

    mysql_close(conn);
	printf("Search row with ID: %d\n", uid);
    return 0;

}




void searchMap(struct Map *myMap,int keyy,unsigned char* data_from_map,int chunk_size){
	for(int i=0;i<myMap->size;i++){
		if(myMap->data[i].key==keyy){
			printf("in Search function:\t");
			data_from_map = myMap->data[i].value;
			memcpy(data_from_map, myMap->data[i].value, chunk_size);
		}
	}
}


void insert(struct Map *myMap,int keyy, unsigned char* valuee,int chunk_size) {
    if (myMap->size < 12) { // Check if there is enough space in the map
		myMap->data[myMap->size].key=keyy;
        // strcpy(myMap->data[myMap->size].key, keyy);
		myMap->data[myMap->size].value = (unsigned char *)calloc(chunk_size, sizeof(unsigned char));
        myMap->data[myMap->size].value = valuee;
		// for(int itr=0;itr<chunk_size;itr++){
		// 	printf("%c",valuee[itr]);
		// }
		myMap->size++;
    } 
	else {
        printf("Map is full, cannot insert more elements.\n");
    }
}


// void str_reverse(char fstr[50])
// {
//     int i=0 ;
//     int j=0 ;
//     int cnt=0 ;
//     char fstr2[50];
    
//     cnt = strlen(fstr) ;
//     for(i=cnt-1, j=0 ; i>=0 ; i--, j++)
//     {
//         fstr2[j] = fstr[i] ;
//         fstr2[j+1] = '\0' ;
//     }  
//     strcpy(fstr, fstr2);
// } 

// void getFileName(char file_path[50], char file_name[50])
// {
// 	int j=0;
// 	int k=0;
// 	str_reverse(file_path);
// 	for(j=0,k=0;file_path[j]!='/' ;j++,k++)
// 	{
// 		file_name[k]=file_path[j];
// 		file_name[k+1] = '\0';
// 	}
// 	file_name[k+1] = '\0';
// 	str_reverse(file_name);
// 	str_reverse(file_path);
// }

// void getFileExtension(char file_name[50], char file_extension[50])
// {
// 	int j=0;
// 	int k=0;
// 	str_reverse(file_name);
// 	for(j=0,k=0;file_name[j]!='.' ;j++,k++)
// 	{
// 		file_extension[k]=file_name[j];
// 		file_extension[k+1] = '\0';
// 	}
// 	file_extension[k+1] = '\0';
// 	str_reverse(file_name);
// 	str_reverse(file_extension);
// }
// int generateFileUid(char file_path[100])
// {
// 	char result[100] = {'\0'};
//  	char command[100] = {'\0'};
// 	int i=0;
// 	int j=0;
// 	int k=0;
// 	int uid_int=0; 
// 	char uid[10]={'\0'};

// 	strcpy(command, "ls -i ");
// 	strcat(command, file_path);
//  	FILE *ls = popen(command, "r");
 	
//  	while(fread(&result[i], sizeof(result[i]), 1, ls))
//  	{
//  		i++;
//  	}    
// 	for(j=0; result[j] != ' '; j++)
// 	{
// 		uid[k] = result[j];
// 		k++;
// 	}
// 	uid_int = atoi(uid);
// 	return uid_int;   
// }


// void calChecksum(char file_path[], char checksum[])
// {
//  	char command[100] = {'\0'}; 
//  	int i=0;
//  	int j=0;
//  	int k=0;
//  	char result[100] = {'\0'};

// 	strcpy(command, "cksum ");
// 	strcat(command, file_path);
//  	FILE *ls = popen(command, "r");
 	
//  	while(fread(&result[i], sizeof(result[i]), 1, ls))
//  	{
//  		i++;
//  	}
 	
//  	for(j=0; result[j] != ' '; j++)
//         {
//         	checksum[k] = result[j];
//         	k++;
//         }

// }


// int isDir(const char *filePath){
// 	DIR *directory=opendir(filePath);
// 	if(directory!=NULL)
// 	{
// 		closedir(directory);
// 		return 0;//is a directory
// 	}
// 	if(errno=ENOTDIR){
// 		return 1;
// 	}
// 	return -1;
// }



