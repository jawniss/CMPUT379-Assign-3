// From: www.thegeekstuff.com/2011/12/c-socket-programming
// Note that port# 5000 is hard-coded into this implementation

/*
okay on bottomright it say s"Col", don't let this go past 80 chars iguess
the way i'm reading it is that with the clients, since each client ==
one terminal, they keep reading in keybaord inputs or read from a file
and then continuously porcess/send the inputs to the server

So we do the same thing as assign 2, continuous input
*/
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 
#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
    /*
    sockfd is the socket, the door
    */
    int sockfd = 0, n = 0;
    char recvBuff[1024];
    struct sockaddr_in serv_addr; 

    if( argc != 3 )
    {
        printf("\n Usage: %s <ip of server> \n",argv[0]);
        return 1;
    } 

    int portNum = stoi( argv[1] );
    int ipAddressInt = stoi( argv[2] );
    char* ipAddressConstChar = argv[2];

    memset( recvBuff, '0', sizeof( recvBuff ) );

    // server can make any socket it wants, but the client
    // has to check if the made socket is the smae as the
    // server socket
    // sockstream: a socket type, theres a bunch of types that do
    // different things
    // failsafe
    if( ( sockfd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
    {
        printf( "\n Error : Could not create socket \n" );
        return 1;
    } 

    memset( &serv_addr, '0', sizeof( serv_addr ) ); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons( portNum ); 

    // i think argv[1] is the ip address passed in
    /*
    if ip address is bad
    */
   // was argv[1] when only inpt was ip address 127.0.0.1
    // if( inet_pton( AF_INET, argv[1], &serv_addr.sin_addr ) <= 0 )
    if( inet_pton( AF_INET, ipAddressConstChar, &serv_addr.sin_addr ) <= 0 )
    {
        printf("\n inet_pton error occured\n");
        return 1;
    } 

    // if it fails to connect
    /*
    ip address + port number are bundled in to the struct and a call to function
    connect() is made, tries to connect this socket with struct socket
    */
    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       printf("\n Error : Connect Failed \n");
       return 1;
    } 

    // if it doesn't read from the socket properly
    // read does not output, returns an int of number of bytes readf
    // read: ( file descripter, into buffer, number of bytes to read )
    // On success, the number of bytes read is returned (zero indicates end
    // of file), and the file position is advanced by this number.
    // i ouputted n, it was 26. so reads all byes at once.

    // n = the nunber of bytes read, and if it's 0 or less it doens' tdo 
    // anything

    while ( ( n = read( sockfd, recvBuff, sizeof( recvBuff ) - 1 ) ) > 0 )
    {
        cout << n << endl;
        // this sets the end of whatever was read into the buffer to zero
        /*
        if buffer reads in a, b and c, n = 3.
        so buffer[3] == 0;
        zero is the terminating character, sending info that this is the end
        sets end of string
        */
        recvBuff[n] = 0;
        // prints everything inside the bufffer
        if( fputs(recvBuff, stdout) == EOF )
        {
            printf("\n Error : Fputs error\n");
        }
    } 

    if(n < 0)
    {
        printf("\n Read error \n");
    } 

    return 0;
}
