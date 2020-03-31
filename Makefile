CFLAGS = -Wall -std=c++11

all: server client
server: server
client: client

# Linker:
# server: server.o tands.o
# 	g++ server.o tands.o -pthread $(CFLAGS) -o server

# client: client.o tands.o
# 	g++ client.o tands.o -pthread $(CFLAGS) -o client

server: server.cpp tands.c
	g++ server.cpp tands.c $(CFLAGS) -o server

client: client.cpp tands.c 
	g++ client.cpp tands.c $(CFLAGS) -o client

# Compilation commands:
# server.o: server.cpp
# 	g++ -c server.cpp $(CFLAGS) -o server.o

# client.o: client.cpp
# 	g++ -c client.cpp $(CFLAGS) -o client.o

# tands.o: tands.c
# 	g++ -c tands.c $(CFLAGS) -o tands.o

clean:
	# @rm -rf server client server.o client.o tands.o
	@rm -rf server client *.o *.log