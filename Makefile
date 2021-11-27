CC=g++
LD=g++
CFLAGS=-g -Wall # debug version
#CFLAGS= -std=c++11 -c -Ofast -march=native -flto -Wall -DNDEBUG -frename-registers -funroll-loops # TODO:release version
CPPFLAGS=-I. -I include
SP_LIBRARY= ./libspread-core.a  ./libspread-util.a

all: server client

.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

server: server.o
	$(LD) -o $@ server.o -ldl $(SP_LIBRARY)

server.o: server.cpp
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

client: client.o
	$(LD) -o $@ client.o -ldl $(SP_LIBRARY)

client.o: client.cpp
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

clean:
	rm -f *.o server client