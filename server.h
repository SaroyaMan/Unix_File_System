#ifndef SERVER_H_
#define SERVER_H_
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include "filedata.h"
#define TMP "locker"
#define PORT "9034"   // port we're listening on

int fdtemp;							
struct flock lock = {F_WRLCK,SEEK_SET,0,0,0};

static void initialize_server() __attribute__((constructor));	//a cunstructor to make sure that maximum servers is 1 !
static void destroy_server() __attribute__((destructor));		//a destructor that unlock a tmp and allow another server to run
void *get_in_addr(struct sockaddr *sa);						
void listing(char* buff);								//loads the buffer with all the files in the Database of server
void loadWithMMAP(FileData* f, char* path);					//load a file from server to a buffer
long unsigned int loadFile(FileData* f);					//load a file from the server depends on operation asked
void saveWithMMAP(FileData* f, char* path);					//stores a file locally in the Database of the server
void storeFile(FileData* f, char* buff);					//stores a file in the server depends on operation asked

#endif /* SERVER_H_  */
