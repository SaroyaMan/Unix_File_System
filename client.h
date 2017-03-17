#ifndef CLIENT_H_
#define CLIENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <libgen.h>
#include "filedata.h"

#define LOCALHOST "127.0.0.1"
#define PORT "9034"
#define MAX_BUFF 1024


void load(FileData* f);
void load_compress(FileData* f);
void load_encode(FileData* f);
int ls_input(FileData* f);
int load_input(FileData* f, char* buffer);
int store_input(FileData* f, char* buffer);


#endif /* CLIENT_H  */
