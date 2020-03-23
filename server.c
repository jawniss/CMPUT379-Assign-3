// From: www.thegeekstuff.com/2011/12/c-socket-programming
// Note that port# 5000 is hard-coded into this implementation

/*
server has to be running the entire time, otherwise the clinets don't have a 
connectino point to go to

first compoile server "gcc server.c -o server"
then compoile clients "gcc client.c -o client"
then run server "./server"
and client "./client 127.0.0.1"
tthen it works, cleitn goes to completion, server stays open
but have to put in the 60 second timer
*/
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 

int main(int argc, char *argv[])
{
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr; 

    char sendBuff[1025];
    time_t ticks; 

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(sendBuff, '0', sizeof(sendBuff)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000);       // hardcoded  port number

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

    listen(listenfd, 10);       // 10 is max num of clients

    while(1)
    {
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL); 

        ticks = time(NULL);
        snprintf(sendBuff, sizeof(sendBuff), "%.24s\r\n", ctime(&ticks));
        write(connfd, sendBuff, strlen(sendBuff)); 

        close(connfd);
        sleep(1);
     }
}
