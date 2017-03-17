#include "client.h"


void load(FileData* f) {

	char destination[100];
	FILE* fp = NULL;

	sprintf(destination,"FilesFromServer/%s",f->filename);
	fp = fopen(destination,"w");
	fwrite(f->data,1,f->data_size,fp);
	fclose(fp);
}

void load_compress(FileData* f) {

	int pipefd[2];
	char destination[100];
	long unsigned int s = 1024;
	int readcount = 0;
	char* tmp = NULL;
	
	if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    	}
	sprintf(destination,"FilesFromServer/%s",f->filename);
	load(f);
		
	if (!fork()) {
		close(pipefd[0]);          /* Close unused read end */
		dup2(pipefd[1],1);//,pipefd[1]);I		
		if (execlp("gunzip","-c","--stdout",destination,NULL) == -1) perror("execlp");
	}
	else {            /* Parent writes argv[1] to pipe */
       	close(pipefd[1]);          /* Reader will see EOF */
		free(f->data);
							
		f->data = (char*) malloc(s);
		tmp = f->data;
		f->data_size = 0;
		while ((readcount = read(pipefd[0],tmp, 512)) > 0) {
			f->data_size+=readcount;
			s = f->data_size*2;
			f->data = (char*) realloc(f->data,s);
			tmp=f->data+f->data_size;
		}
		wait(NULL);                /* Wait for child */
		load(f);
    	}
	printf("The file has been loaded and uncompressed successfully\n");
}

void load_encode(FileData* f) {

	int pipefd[2];
	char destination[100];
	long unsigned int s = 1024;
	int readcount = 0;
	char* tmp = NULL;

	if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    	}
	sprintf(destination,"FilesFromServer/%s",f->filename);
	load(f);
	
	if (!fork()) {
		close(pipefd[0]);          /* Close unused read end */
		dup2(pipefd[1],1);//,pipefd[1]);I		
	
		chdir("FilesFromServer");
		if (!fork()) {if (execlp("uudecode","uudecode",f->filename,NULL) == -1) perror("execlp");}
		else {sleep(5); if (execlp("mv","--force","tmp",f->filename,NULL) == -1) perror("execlp");}	
	}
	printf("The file has been loaded and decoded successfully\n");
}


int ls_input(FileData* f) {

	f->operation = strdup("LS");
	f->filename = strdup("0");
	f->data = strdup("0");
	f->filename_size = strlen(f->filename)+1;
	f->data_size = strlen(f->data)+1;
	f->operation_size = strlen(f->operation)+1;
	return 1;
}

int load_input(FileData* f, char* buffer) {
	int offset = 5;
	char op[15];
	if (!strncmp(buffer+offset,"COMPRESS ",9)) {offset+=9;strcpy(op,"LOAD COMPRESS");}
	else if (!strncmp(buffer+offset,"ENCODE ",7)) {offset+=7;strcpy(op,"LOAD ENCODE");}
	else strcpy(op,"LOAD");
	initializeFileData(f,basename(buffer+offset),op);
	f->data = strdup("0");
	f->data_size = strlen(f->data)+1;
	if (f->filename[0] == '.' || !strcmp(f->filename,"tmp") || f->filename[0] == '/') return 0;
	return 1;
}

int store_input(FileData* f, char* buffer) {

	int pipefd[2];
    	pid_t cpid;
	int readcount = 0;
	char* tmp = NULL;
	long unsigned int s = 1024;

	int offset = 6;
	char zipper[MAX_BUFF+10],encoder[MAX_BUFF+10] = {0};
	FILE* fp = NULL;
	
	if (!strncmp(buffer+offset,"COMPRESS ",9) && buffer[offset+10] != '\0') {			//compressing the file
		offset+=9;
		if (!file_exist(buffer + offset) || !strcmp(basename(buffer+offset),"tmp"))
			{fprintf(stderr,"Error: The file is not exists. "); return 0;}
		initializeFileData(f,basename(buffer+offset),"STORE COMPRESS");		

		if (pipe(pipefd) == -1) {
        		perror("pipe");
        		exit(EXIT_FAILURE);
    		}
    		cpid = fork();
    		if (cpid == -1) {
        		perror("fork");
        		exit(EXIT_FAILURE);
    		}
    		if (cpid ==0 ) {    /* Child read */
			close(pipefd[0]);          /* Close unused read end */
			dup2(pipefd[1],1);//,pipefd[1]);I
			if (execlp("gzip","-c","--stdout",buffer+offset,NULL)==-1) perror("execlp");
      	}					// _exit(EXIT_SUCCESS);
		else {            /* Parent writes argv[1] to pipe */
			
        		close(pipefd[1]);          /* Reader will see EOF */

			f->data = (char*) malloc(s);	// s = 1024
			tmp = f->data;		
			while ((readcount = read(pipefd[0],tmp, 512)) > 0) {
				f->data_size+=readcount;
				s = f->data_size*2;
				f->data = (char*) realloc(f->data,s);
				tmp=f->data+f->data_size;
			}

			return 1;
    		}
	}
	else if (!strncmp(buffer+offset,"ENCODE ",7) && buffer[offset+8] != '\0') {			//encoding the file
		offset+=7;
		if (!file_exist(buffer + offset) || !strcmp(basename(buffer+offset),"tmp"))
			{fprintf(stderr,"Error: The file is not exists. "); return 0;}
		initializeFileData(f,basename(buffer+offset),"STORE ENCODE");			

		if (pipe(pipefd) == -1) {
        		perror("pipe");
        		exit(EXIT_FAILURE);
    		}
    		cpid = fork();
    		if (cpid == -1) {
        		perror("fork");
        		exit(EXIT_FAILURE);
    		}
    		if (cpid ==0 ) {    /* Child read */
			close(pipefd[0]);          /* Close unused read end */
			dup2(pipefd[1],1);//,pipefd[1]);I
			if (execlp("uuencode","-m",buffer+offset,"tmp",NULL)==-1) perror("execlp");
      	}					// _exit(EXIT_SUCCESS);
		else {            /* Parent writes argv[1] to pipe */

        		close(pipefd[1]);          /* Reader will see EOF */
			
			f->data = (char*) malloc(s);	// s = 1024
			tmp = f->data;
			while ((readcount = read(pipefd[0],tmp, 512)) > 0) {
				f->data_size+=readcount;
				s = f->data_size*2;
				f->data = (char*) realloc(f->data,s);
				tmp=f->data+f->data_size;
			}
			return 1;
    		}		
	}
	else {
		if (!(fp = fopen(buffer+offset,"r")) || !strcmp(basename(buffer+offset),"tmp")) {
			fprintf(stderr, "Error: The file could not be opened. ");
			return 0;
		}
		if (!is_regular_file(buffer+offset) || buffer[offset]== ' ') {
			fprintf(stderr,"Error: This program only manipulates regular files. ");
			return 0;
		}

		initializeFileData(f,basename(buffer+offset),"STORE");	
		f->data_size = getFileSize(f->filename);			//copy the data_size
		f->data = (char*) malloc(f->data_size);
		fread(f->data,1,f->data_size,fp);					//copy the data
		fclose(fp);
		return 1;
	}
}


int main(int argc, char * argv[]) {

	int load_flag = 0,rv=0, nbytes = 0, sockfd = -1;
	struct addrinfo hints, *servinfo, *p;
	char buffer[MAX_BUFF + 1] = {0};
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6
	hints.ai_socktype = SOCK_STREAM;
	fd_set readfds, allfds;

	FILE* fp = NULL;
	char *data,*file_bytes, *temp, *address;
	FileData file_data;
	long unsigned int size_send = 0;

	char answer, zipper[MAX_BUFF+10];
	int success = 0;	

	if (argc != 2) address = LOCALHOST;
	else address = argv[1];
	printf("Connecting to %s\n",address);
	if ((rv = getaddrinfo(address, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(1);
	}
	temp = (char*) malloc(sizeof(long unsigned int));
	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
			perror("socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("connect");
			close(sockfd);
			continue;
		}
		break; // if we get here, we must have connected successfully
	}

	freeaddrinfo(servinfo);	
	FD_ZERO(&readfds);
	FD_SET(0, &allfds);
	FD_SET(sockfd, &allfds);
	for (;;) {
		readfds = allfds;
		if (select(sockfd + 1, &readfds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(4);
		}
		if (FD_ISSET(0, &readfds)) {
			fgets(buffer, MAX_BUFF, stdin);
			fflush(stdin);	fflush(stdout);
			buffer[strlen(buffer)-1] = '\0';
			if (!strcmp(buffer,"EXIT")) break;								//the client inputed EXIT
			else if (!strncmp(buffer,"STORE ",6)) success = store_input(&file_data,buffer);	//client inputed STORE
			else if(!strcmp(buffer,"LS"))	success = ls_input(&file_data);		//client inputed LS 				
			else if(load_flag = !strncmp(buffer,"LOAD ",5))success = load_input(&file_data,buffer);//client inputed LOAD	
			else {
				fprintf(stderr,"You should send only 'LS', 'LOAD', 'STORE' 'EXIT' commands\n");
				continue;
			}
			if (success) {
				data = fileDataToBuffer(&file_data);
				size_send = calculateFileDataSize(&file_data)+8;
				if (file_data.data_size == 0) {
					fprintf(stderr,"You can't send empty files\n");
				}
				else nbytes = send(sockfd,data,size_send,0);
				clearFileData(&file_data);
			}
			else {printf("Please try again\n");load_flag = 0;	continue;}
		}
		if (FD_ISSET(sockfd, &readfds)) {
			if (load_flag) {
				load_flag = 0;
				nbytes = recv(sockfd, temp, sizeof(long unsigned int),0);
				memcpy(&size_send,temp,sizeof(size_send));
				
				file_bytes = (char*) malloc(size_send+1);
				recv(sockfd,file_bytes,size_send,MSG_WAITALL);
				bufferToFileData(file_bytes,&file_data);
				if (!strncmp(file_data.operation,"LOAD",4)) {
					if (!file_exist("FilesFromServer")) {
   						mkdir("FilesFromServer", 0700);
						printf("FilesFromServer directory has been created\n");
					}

					if (!strcmp(file_data.operation,"LOAD COMPRESS")) {
							
						load_compress(&file_data);
					}
					else if (!strcmp(file_data.operation,"LOAD ENCODE"))
						load_encode(&file_data);

					else {
						load(&file_data);
						printf("The file has been loaded successfully\n");
					}
				}
				else printf("The file %s is not exists in the server\n",file_data.filename);
				clearFileData(&file_data);
				continue;
			}
			nbytes = recv(sockfd, buffer, MAX_BUFF, 0);
			if (nbytes <= 0) {
				if (nbytes == 0) {
					fprintf(stderr,"connection to server on socket %d lost\n", sockfd);
					exit(1);
				}
				else perror("recv");
				close(sockfd);
				FD_CLR(sockfd, &allfds);
			}
			else {
				buffer[nbytes] = '\0';
				puts(buffer);
			}
		}
	}
	return 0;
}
