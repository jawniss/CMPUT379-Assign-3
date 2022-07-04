CFLAGS = -Wall -std=c++11

all: server client serverMan clientMan
server: server
client: client
serverMan: serverMan
clientMan: clientMan

server: server.cpp tands.c
	g++ server.cpp tands.c $(CFLAGS) -o server

client: client.cpp tands.c 
	g++ client.cpp tands.c $(CFLAGS) -o client

clientMan:
	groff -man -Tpdf < client.1  > client.pdf

serverMan:
	groff -man -Tpdf < server.1  > server.pdf

clean:
	@rm -rf server client *.o *.log *.pdf
	# Windows:
	# -del server client *.o *.log *.pdf
