#ifndef FILEDATA_H_
#define FILEDATA_H_
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

/* the struct represents a File that about to be sent to/from the server*/
typedef struct FileData {

	size_t filename_size;
	size_t data_size;		
	size_t operation_size;

	char* filename;
	char* data;
	char* operation;

}FileData;

void initializeFileData(FileData* f, char* name, char* oper);	//initialize the FileData
void clearFileData(FileData* fd);							//clear the FileData (destructor)
void copyFileData(FileData* target, FileData* source);			//copy constructor of FileData
void printFileData(FileData* fd);							//print method of FileData
int file_exist(char *filename);							//check if a file exist in the computer
int is_regular_file(const char *path);						//check if the file is a regular file (and not a directory)
long unsigned int getFileSize(char* filename);				//get the file size of a file
long unsigned int calculateFileDataSize(FileData* fd);			//get the size of the FileData in bytes
char* fileDataToBuffer(FileData* fd);						//serialize the FileData to a buffer
void bufferToFileData(char* buffer, FileData* fd);			//unserialize the buffer to a FileData

#endif /* FILEDATA_H_ */
