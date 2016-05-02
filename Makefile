COMPILER=g++
CPPFLAGS=-Wall -Wunused -Wshadow -pedantic -O2 -std=c++11 -g#-DDEBUG

all: server client

server: server.cpp utility.o
	$(COMPILER) $(CPPFLAGS) -o server server.cpp utility.o

client: client.cpp utility.o
	$(COMPILER) $(CPPFLAGS) -o client client.cpp utility.o

utility.o: utility.h utility.cpp
	$(COMPILER) $(CPPFLAGS) -c utility.cpp

clean:
	rm -f client server *.o
