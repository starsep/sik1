COMPILER=g++
CPPFLAGS=-Wall -Wunused -Wshadow -pedantic -O3 -std=c++11 -DDEBUG -g

all: server client

server: server.cpp utility.o Debug.o
	$(COMPILER) $(CPPFLAGS) -o server server.cpp utility.o Debug.o

client: client.cpp utility.o Debug.o
	$(COMPILER) $(CPPFLAGS) -o client client.cpp utility.o Debug.o

utility.o: utility.h utility.cpp Debug.o
	$(COMPILER) $(CPPFLAGS) -c utility.cpp

Debug.o: Debug.h Debug.cpp
	$(COMPILER) $(CPPFLAGS) -c Debug.cpp

clean:
	rm -f client server *.o
