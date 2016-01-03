cc = gcc
ccflags = -g -I. -std=gnu99 -pthread -DDEBUG

all:	curlychatbot

curlychatbot: netcode.o curlychatbot.o login.o
	$(cc) ${ccflags} netcode.o curlychatbot.o login.o -o curlychatbot -ljansson -lcurl

curlychatbot.o: curlychatbot.c curlychatbot.h
	$(cc) curlychatbot.c -c ${ccflags} $(pkg-config --cflags jansson) $(pkg-config --cflags libcurl)

netcode.o:	netcode.c netcode.h
	$(cc) netcode.c -c ${ccflags} $(pkg-config --cflags jansson) $(pkg-config --cflags libcurl)

login.o:	login.c login.h
	$(cc) login.c -c ${ccflags} $(pkg-config --cflags jansson) $(pkg-config --cflags libcurl)

clean:
	/bin/rm -f *.o curlychatbot