#include "server.h"

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


void listing(char* buff) {
	int flag=1;
	memset(buff,0,strlen(buff));
	DIR* mydir;
	struct dirent *myfile;
	for(mydir = opendir("Database"); (myfile = readdir(mydir)) != NULL; ) {
		if (myfile->d_name[0] != '.' && strcmp(myfile->d_name,"tmp")) {
			strcat(buff,myfile->d_name);
			strcat(buff,"  ");
			flag=0;
		}
	}
	if (flag) sprintf(buff,"The server is empty");
	closedir(mydir);
}


long unsigned int loadFile(FileData* f) {

	unsigned char* buf = NULL;
	int readcount = 0;
	char* tmp = NULL;
	long unsigned int s = 1024;

	char path[100],encoder[100], zipper[100];
	pid_t  cpid;
	int pipefd[2], size = 0, remover = 0 , process_needed = 0;
	if (!strncmp(f->operation,"LOAD ENCODE",11) || !strncmp(f->operation,"LOAD COMPRESS",13)) process_needed = 1;
	sprintf(path,"Database/%s",f->filename);
	if (file_exist(path) && f->filename[0] != '.') {

		if (process_needed) {

			if (pipe(pipefd) == -1) {
       			perror("pipe");
      	  		exit(EXIT_FAILURE);
  			}
			cpid = fork();
    			if (cpid == -1) {
        			perror("fork");
        			exit(EXIT_FAILURE);
    			}

			if (!strncmp(f->operation,"LOAD COMPRESS",13)) {
				if (!cpid) {    /* Child read */
				close(pipefd[0]);          /* Close unused read end */
				dup2(pipefd[1],1);//,pipefd[1]);I
				if (execlp("gzip","-c","--stdout",path,NULL)==-1) perror("execlp");
      			}					// _exit(EXIT_SUCCESS);
				else {            /* Parent writes argv[1] to pipe */
        				close(pipefd[1]);          /* Reader will see EOF */
					free(f->data);	f->data_size = 0;
					f->data = (char*) malloc(s);	// s = 1024
					tmp = f->data;		
					while ((readcount = read(pipefd[0],tmp, 512)) > 0) {
						f->data_size+=readcount;
						s = f->data_size*2;
						f->data = (char*) realloc(f->data,s);
						tmp=f->data+f->data_size;
					}
					return calculateFileDataSize(f)+8;
    				}
			}

			else if (!strncmp(f->operation,"LOAD ENCODE",11)) {
				
				if (!cpid) {    /* Child read */
					close(pipefd[0]);          /* Close unused read end */
					dup2(pipefd[1],1);//,pipefd[1]);I
					chdir("Database");
					if (execlp("uuencode","-m",f->filename,"tmp",NULL)==-1) perror("execlp");
      			}					// _exit(EXIT_SUCCESS);
				else {            /* Parent writes argv[1] to pipe */
        				close(pipefd[1]);          /* Reader will see EOF */
					free(f->data);	f->data_size = 0;
					f->data = (char*) malloc(s);	// s = 1024
					tmp = f->data;		
					while ((readcount = read(pipefd[0],tmp, 512)) > 0) {
						f->data_size+=readcount;
						s = f->data_size*2;
						f->data = (char*) realloc(f->data,s);
						tmp=f->data+f->data_size;
					}
				return calculateFileDataSize(f)+8;
    				}
			}
		}
		loadWithMMAP(f,path);
	}
	else {
		memset(f->operation,0,f->operation_size);
		f->operation = strdup("FAIL");
		f->operation_size = strlen(f->operation)+1;
	}
	return calculateFileDataSize(f)+8;
}



void storeFile(FileData* f, char* buff) {

	int pipefd[2], i;
	unsigned char* buf = NULL;
	int readcount = 0;
	char* tmp = NULL;
	char path[100], encode[100];
	int fd = 0, process_needed = 0, encoded = 0;

	memset(buff,0,strlen(buff));
	sprintf(path,"Database/%s",f->filename);
	if (!strcmp(f->operation,"STORE COMPRESS") || (encoded = !strcmp(f->operation,"STORE ENCODE"))) process_needed = 1;
	if (file_exist(path)) sprintf(buff,"The file %s is already exists in the server!",f->filename);
	else {
		saveWithMMAP(f,path);
		if (process_needed) {
			
			if (pipe(pipefd) == -1) {
        				perror("pipe");
        				exit(EXIT_FAILURE);
    			}			
			if (!fork()) {
				
				close(pipefd[0]);          /* Close unused read end */
				
				dup2(pipefd[1],1);//,pipefd[1]);I			
			
				//uncompressing the file
				if (!strcmp(f->operation,"STORE COMPRESS")) {
					if (execlp("gunzip","-c","--stdout",path,NULL) == -1) perror("execlp");
				}
				//decoding the file
				else if(!strcmp(f->operation,"STORE ENCODE")) {
					chdir("Database");
					if (!fork()) {if (execlp("uudecode","uudecode",f->filename,NULL) == -1) perror("execlp");}
					else {sleep(5); if (execlp("mv","--force","tmp",f->filename,NULL) == -1) perror("execlp");}
				}
			}
			else {      					/* Parent writes argv[1] to pipe */
				if (encoded) {
					printf("The file %s has been decoded\n",f->filename);
				}
				else if ( !strcmp(f->operation,"STORE COMPRESS")) {
					close(pipefd[1]);          /* Reader will see EOF */
					free(f->data);
					long unsigned int s = 1024;
					f->data = (char*) malloc(s);
					tmp = f->data;
					f->data_size = 0;
					while ((readcount = read(pipefd[0],tmp, 512)) > 0) {
						f->data_size+=readcount;
						s = f->data_size*2;
						f->data = (char*) realloc(f->data,s);
						tmp=f->data+f->data_size;
					}
					saveWithMMAP(f,path);
					printf("The file %s has been uncompressed\n",f->filename);
				}  
    			}
		}
		sprintf(buff,"The file %s has been wrotten into the server", f->filename);
	}
}

static void initialize_server() {

	fdtemp = open(TMP, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
	if (fcntl(fdtemp,F_SETLKW,&lock) == -1) {
		perror("fcntl");
		exit(1);
	}
}

static void destroy_server() {

	lock.l_type = F_UNLCK;
	if (fcntl(fdtemp,F_SETLKW,&lock) == -1) {
		perror("fcntl");
		exit(1);
	}
	close(fdtemp);
	remove(TMP);
}

int main(int argc, char** argv) {

	FileData file_data;
	long unsigned int size = 0;
	char* buf = NULL;		// buffer for client data
	char* temp = (char*) malloc(sizeof(long unsigned int));

    	fd_set master;    // master file descriptor list
    	fd_set read_fds;  // temp file descriptor list for select()
    	int fdmax;        // maximum file descriptor number
	int nbytes;
    	int listener;     // listening socket descriptor
    	int newfd;        // newly accept()ed socket descriptor
    	struct sockaddr_storage remoteaddr; // client address
    	socklen_t addrlen;

    	char remoteIP[INET6_ADDRSTRLEN];

    	int yes=1;        // for setsockopt() SO_REUSEADDR, below
    	int i,rv;

	struct addrinfo hints, *ai, *p;

	if (!file_exist("Database")) {
   		mkdir("Database", 0700);
		printf("Database has been created\n");
	}

    	FD_ZERO(&master);    // clear the master and temp sets
    	FD_ZERO(&read_fds);

	// get us a socket and bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
		fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
		exit(1);
	}
	
	for(p = ai; p != NULL; p = p->ai_next) {
    		listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener < 0)  continue;
		// lose the pesky "address already in use" error message
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
			close(listener);
			continue;
		}
		break;
	}

	// if we got here, it means we didn't get bound
	if (p == NULL) {
		fprintf(stderr, "selectserver: failed to bind\n");
		exit(2);
	}
	freeaddrinfo(ai); // all done with this

    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one
	fprintf(stdout,"Server has been created and is ready for clients requests\n");
    // main loop
	for(;;) {
		read_fds = master;	 // copy it
        	if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
          	perror("select");
            	exit(4);
        	}
        	// run through the existing connections looking for data to read
        	for(i = 0; i <= fdmax; i++) {
            	if (FD_ISSET(i, &read_fds)) { // we got one!!
            		if (i == listener) {
                    	// handle new connections
                    	addrlen = sizeof remoteaddr;
					newfd = accept(listener,(struct sockaddr *)&remoteaddr,&addrlen);
					if (newfd == -1)  perror("accept");
                   	 	else {
                   			FD_SET(newfd, &master); // add to master set
                      	 	if (newfd > fdmax) fdmax = newfd;    // keep track of the max
                          	printf("selectserver: new connection from %s on "
                            "socket %d\n", inet_ntop(remoteaddr.ss_family,get_in_addr((struct sockaddr*)&remoteaddr),remoteIP, 							INET6_ADDRSTRLEN),newfd);
                    	}
                	}
			 	else {
                    // handle data from a client
                    	if ((nbytes = recv(i, temp, sizeof(long unsigned int), 0)) <= 0) {
                        // got error or connection closed by client
                        		if (nbytes == 0) printf("selectserver: socket %d hung up\n", i); // connection closed
                        		else perror("recv");
                        		close(i); // bye!
                        		FD_CLR(i, &master); // remove from master set
                    	}
					else { 	   // we got some data from a client
						memcpy(&size,temp,sizeof(size));
						buf = (char*) malloc(size+1);
						recv(i,buf,size,MSG_WAITALL);
						bufferToFileData(buf,&file_data);
				 		if (!strncmp(file_data.operation,"LS",2)) {
							printf("socket %d has been asked to LS\n",i);
							listing(buf);
							size = strlen(buf)+1;
				 		}
						else if(!strncmp(file_data.operation,"LOAD",4)) {
							printf("socket %d has been asked to %s the file %s		\n",i,file_data.operation,file_data.filename);
							size = loadFile(&file_data); 
							buf = fileDataToBuffer(&file_data);
						}
						else if(!strncmp(file_data.operation,"STORE",5)) {
							printf("socket %d has been asked to %s the file %s		\n",i,file_data.operation,file_data.filename);
							storeFile(&file_data,buf);
							size = strlen(buf)+1;
						}
						if (send(i, buf, size, 0) == -1) perror("send");
						clearFileData(&file_data);
                    	}
				
                	} // END handle data from client
            	} // END got new incoming connection
        	} // END looping through file descriptors
    	} // END for(;;)--and you thought it would never end!
	return 0;
}


void saveWithMMAP(FileData* f, char* path) {

	int fd = 0;
	char* map;
	lock.l_type = F_WRLCK;

	fd = open(path, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
   	if (fd == -1) {
      	perror("Error opening file for writing");
      	exit(EXIT_FAILURE);
    	}
	
 	fcntl(fd,F_SETLKW,&lock); //locking the file

	if (lseek(fd, f->data_size-1, SEEK_SET) == -1) {
          close(fd);
          perror("Error calling lseek() to 'stretch' the file");
        	exit(EXIT_FAILURE);
    	}
  	if (write(fd, "", 1) == -1) {
      	close(fd);
        	perror("Error writing last byte of the file");
        	exit(EXIT_FAILURE);
  	}
    
   	 // now the file is ready to be mmapped.
    	map = mmap(0, f->data_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    	if (map == MAP_FAILED) {
        	close(fd);
		perror("Error mmapping the file");
        	exit(EXIT_FAILURE);
    	}
    	memcpy(map,f->data,f->data_size);

    	// write it now to disk
    	if (msync(map, f->data_size, MS_SYNC) == -1)
        	perror("Could not sync the file to disk");
   
    	// don't forget to free the mmapped memory
    	if (munmap(map, f->data_size) == -1) {
        	close(fd);
        	perror("Error un-mmapping the file");
        	exit(EXIT_FAILURE);
    	}

	/*releasing the file*/
	lock.l_type = F_UNLCK;
  	fcntl(fd,F_SETLKW,&lock);
		
	close(fd);
}


void loadWithMMAP(FileData* f, char* path) {

	struct stat fileInfo = {0};
	int fd = 0;
	char* tmp;

	lock.l_type = F_RDLCK;

	memset(f->data,0,f->data_size);
	f->data_size = getFileSize(path);
	f->data = (char*) malloc(f->data_size);

	fd = open(path,O_RDONLY, (mode_t)0600);
	if (fd == -1) {
      	perror("Error opening file for writing");
          exit(EXIT_FAILURE);
   	}   
	
 	fcntl(fd,F_SETLKW,&lock);	//locking the file

	if (fstat(fd, &fileInfo) == -1) {
      	perror("Error getting the file size");
       	exit(EXIT_FAILURE);
    	} 
	if (fileInfo.st_size == 0){
     	fprintf(stderr, "Error: File is empty, nothing to do\n");
      	exit(EXIT_FAILURE);
   	}	
	tmp = mmap(0, fileInfo.st_size, PROT_READ, MAP_SHARED, fd, 0);
    	if (tmp  == MAP_FAILED) {
          close(fd);
       	perror("Error mmapping the file");
        	exit(EXIT_FAILURE);
   	}
	memcpy(f->data, tmp, f->data_size);
	if (munmap(tmp, fileInfo.st_size) == -1) {
       	close(fd);
       	perror("Error un-mmapping the file");
        	exit(EXIT_FAILURE);
   	}

	/*releasing the file*/
	lock.l_type = F_UNLCK;
  	fcntl (fd, F_SETLKW, &lock);
	
	close(fd);
}
