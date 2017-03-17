all: client server filedata
	gcc -g client.o filedata.o -o client #compile client object into exec file
	gcc -g server.o filedata.o -o server #compile server object into exec file


client: client.c
	gcc -g -c client.c -o client.o #compile client into object file

server: server.c
	gcc -g -c server.c -o server.o #compile server into object file

filedata: filedata.c
	gcc -g -c filedata.c -o filedata.o

clean:
	rm -f locker client server filedata *.o #remove all compilation products
