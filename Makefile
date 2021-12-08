CC=g++
LD=g++
CFLAGS=-g -std=c++11 -Wall

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

clean_log:
	rm *.log *.state *.know *tmp *.timestamp *.aux

