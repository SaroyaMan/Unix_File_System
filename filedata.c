#include "filedata.h"


void initializeFileData(FileData* f, char* name, char* oper) {

	f->filename = strdup(name);	
	f->operation = strdup(oper);

	f->filename_size = strlen(f->filename)+1;
	f->operation_size = strlen(f->operation)+1;
	f->data = NULL;
	f->data_size = 0;
}


void clearFileData(FileData* fd) {
	
	memset(fd->filename,0,fd->filename_size);
	memset(fd->data,0,fd->data_size);
	memset(fd->operation,0,fd->operation_size);
	fd->filename_size = 0;
	fd->data_size = 0;
	fd->operation_size = 0;
}

void copyFileData(FileData* target, FileData* source) {
	
	target->filename_size = source->filename_size;
	target->operation_size = source->operation_size;
	target->data_size = source->data_size;
	target->filename = strdup(source->filename);
	target->operation = strdup(source->operation);
	memcpy(target->data, source->data, target->data_size);
}

void printFileData(FileData* fd) {

	printf("Size of the filename: %zu\n",fd->filename_size);
	printf("Size of the data: %zu\n",fd->data_size);
	printf("Size of the operation: %zu\n", fd->operation_size);
	printf("Name of the file: %s\n",fd->filename);
	printf("Data of the file: %s\n",fd->data);
	printf("Operation: %s\n",fd->operation);
}

long unsigned int getFileSize(char* filename) {
	struct stat st;
	stat(filename,&st);
	return st.st_size;
}

int file_exist(char *filename) {
  struct stat buffer;   
  return (!stat(filename, &buffer));
}

int is_regular_file(const char *path) {
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

long unsigned int calculateFileDataSize(FileData* fd) {
	int size = 0;
	if (fd)
		size = sizeof(size_t)*3 + fd->filename_size + fd->data_size + fd->operation_size+3;
	return size;
}

char* fileDataToBuffer(FileData* fd) {

	char *buffer=NULL, *temp=NULL;
	int sz = sizeof(size_t);
	long unsigned int s = 0;
	int j = 0; 
	if (fd) {
		s = calculateFileDataSize(fd);
		buffer = (char*)malloc(s+sizeof(long unsigned int));
		temp = buffer;
		memcpy(temp, &s, sizeof(long unsigned int));
		temp+=sizeof(long unsigned int);
		memcpy(temp, &fd->filename_size, sz);
		temp+= sz;
		memcpy(temp, &fd->data_size, sz);
		temp+= sz;
		
		memcpy(temp, &fd->operation_size, sz);
		temp+= sz;
		
		memcpy(temp, fd->filename, fd->filename_size);
		temp+= fd->filename_size;
		memcpy(temp, fd->data, fd->data_size);

		temp+=fd->data_size;
		memcpy(temp, fd->operation, fd->operation_size);
	//	printf("SIZE = %lu\n", calculateFileDataSize(fd));
	//	printFileData(fd);
	}
	return buffer;
}

void bufferToFileData(char* buffer, FileData* fd) {

	char* temp = buffer;
	int j = 0;
	if (buffer) {
		memcpy(&fd->filename_size,temp,sizeof(size_t));
		temp+=sizeof(fd->filename_size);
		memcpy(&fd->data_size,temp,sizeof(size_t));
		temp+=sizeof(fd->data_size);
		memcpy(&fd->operation_size,temp,sizeof(size_t));
		temp+=sizeof(fd->operation_size);
		fd->filename = (char*) malloc(fd->filename_size+1);
		fd->data = (char*) malloc(fd->data_size+1);
		fd->operation = (char*) malloc(fd->operation_size+1);

		memcpy(fd->filename,temp,fd->filename_size);
		temp+=fd->filename_size;
		memcpy(fd->data,temp,fd->data_size);
		temp+=fd->data_size;
		memcpy(fd->operation,temp,fd->operation_size);
	}
}
