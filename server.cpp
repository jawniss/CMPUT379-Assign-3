/*------------------------------------------------------
Adapted from 
https://www.ibm.com/support/knowledgecenter/ssw_ibm_i_71/rzab6/poll.htm
------------------------------------------------------*/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <time.h> 
#include <iostream>
#include <sys/time.h>
#include <iomanip>


using namespace std;

void Trans( int n );    // Forward declarations of the provided functions without using header file
void Sleep( int n );

int    len, rc, on = 1, timeout, transactionsDone = 0;
int    listen_sd = -1, new_sd = -1;
int    desc_ready, end_server = false, compress_array = false;
int    close_conn;
char   buffer[1025];
char   sendBuff[1025];
struct sockaddr_in   addr;
struct pollfd fds[200];
int    nfds = 1, current_size = 0, i, j;
int    totalTrans = 0;


void printEpochTime()
{
    struct timeval tv;

    // epoch time in ms
    gettimeofday(&tv,NULL);
    unsigned long long seconds = tv.tv_sec;
    unsigned long long millisecs = tv.tv_usec / 10000;
    cout << seconds << "." << millisecs;
}


void setup( int argc, char *argv[] )
{
    if( argc != 2 )     // maybe later i should change these to ask again? possible?
    {
        cout << "Invalid input arguments" << endl;
        exit( EXIT_FAILURE );
    }

    int portNum = stoi( argv[1] );

    if( portNum < 5000 or portNum > 64000 )
    {
        cout << "Port number must be between 5000 and 64,000" << endl;
        exit( EXIT_FAILURE );
    } else {
        cout << "Port number selected: " << portNum << endl;
    }
    
    /*************************************************************/
    /* Create an AF_INET6 stream socket to receive incoming      */
    /* connections on                                            */
    /*************************************************************/
    listen_sd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sd < 0)
    {
        perror("socket() failed");
        exit(-1);
    }

    /*************************************************************/
    /* Allow socket descriptor to be reuseable                   */
    /*************************************************************/
    rc = setsockopt(listen_sd, SOL_SOCKET,  SO_REUSEADDR,
                    (char *)&on, sizeof(on));
    if (rc < 0)
    {
        perror("setsockopt() failed");
        close(listen_sd);
        exit(-1);
    }

    /*************************************************************/
    /* Set socket to be nonblocking. All of the sockets for      */
    /* the incoming connections will also be nonblocking since   */
    /* they will inherit that state from the listening socket.   */
    /*************************************************************/
    rc = ioctl(listen_sd, FIONBIO, (char *)&on);
    if (rc < 0)
    {
        perror("ioctl() failed");
        close(listen_sd);
        exit(-1);
    }

    /*************************************************************/
    /* Bind the socket                                           */
    /*************************************************************/
    memset(&addr, 0, sizeof(addr));
    memset( sendBuff, '0', sizeof( sendBuff ) );
    memset( buffer, '0', sizeof( buffer ) );
    addr.sin_family      = AF_INET;
    // memcpy(&addr.sin_addr, &in6addr_any, sizeof(in6addr_any));
    addr.sin_port        = htons( portNum );
    rc = ::bind(listen_sd, (struct sockaddr *)&addr, sizeof(addr));
    if (rc < 0)
    {
        perror("bind() failed");
        close(listen_sd);
        exit(-1);
    }

    /*************************************************************/
    /* Set the listen back log                                   */
    /*************************************************************/
    rc = listen(listen_sd, 32);
    if (rc < 0)
    {
        perror("listen() failed");
        close(listen_sd);
        exit(-1);
    }

    /*************************************************************/
    /* Initialize the pollfd structure                           */
    /*************************************************************/
    memset( fds, 0 , sizeof( fds ) );

    /*************************************************************/
    /* Set up the initial listening socket                        */
    /*************************************************************/
    fds[0].fd = listen_sd;
    fds[0].events = POLLIN;
    /*************************************************************/
    /* Initialize the timeout to 60 seconds. If no                */
    /* activity after 60 seconds this program will end.           */
    /* timeout value is based on milliseconds.                   */
    /*************************************************************/
    timeout = ( 60 * 1000 );
}


void serverLoop()
{
    /*************************************************************/
    /* Loop waiting for incoming connects or for incoming data   */
    /* on any of the connected sockets.                          */
    /*************************************************************/
    do
    {
        /***********************************************************/
        /* Call poll() and wait 60 seconds for it to complete.      */
        /***********************************************************/
        printf("Waiting on poll()...\n");
        rc = poll(fds, nfds, timeout);

        /***********************************************************/
        /* Check to see if the poll call failed.                   */
        /***********************************************************/
        if (rc < 0)
        {
        perror("  poll() failed");
        break;
        }

        /***********************************************************/
        /* Check to see if the 60 second time out expired.          */
        /***********************************************************/
        if (rc == 0)
        {
        printf("  poll() timed out.  End program.\n");
        break;
        }


        /***********************************************************/
        /* One or more descriptors are readable.  Need to          */
        /* determine which ones they are.                          */
        /***********************************************************/
        current_size = nfds;
        for (i = 0; i < current_size; i++)
        {
            /*********************************************************/
            /* Loop through to find the descriptors that returned    */
            /* POLLIN and determine whether it's the listening       */
            /* or the active connection.                             */
            /*********************************************************/
            if(fds[i].revents == 0)
                continue;

            if (fds[i].fd == listen_sd)
            {
                /*******************************************************/
                /* Listening descriptor is readable.                   */
                /*******************************************************/
                // This is printed everytime a new client is connected
                printf("  Listening socket is readable\n");

                /*******************************************************/
                /* Accept all incoming connections that are            */
                /* queued up on the listening socket before we         */
                /* loop back and call poll again.                      */
                /*******************************************************/
                do
                {
                    /*****************************************************/
                    /* Accept each incoming connection. If               */
                    /* accept fails with EWOULDBLOCK, then we            */
                    /* have accepted all of them. Any other              */
                    /* failure on accept will cause us to end the        */
                    /* server.                                           */
                    /*****************************************************/
                    new_sd = accept(listen_sd, NULL, NULL);
                    if (new_sd < 0)
                    {
                        if (errno != EWOULDBLOCK)
                        {
                            perror("  accept() failed");
                            end_server = true;
                        }
                        break;
                    }

                    /*****************************************************/
                    /* Add the new incoming connection to the            */
                    /* pollfd structure                                  */
                    /*****************************************************/
                    printf("  New incoming connection - %d\n", new_sd);
                    fds[nfds].fd = new_sd;
                    fds[nfds].events = POLLIN;
                    nfds++;

                /*****************************************************/
                /* Loop back up and accept another incoming          */
                /* connection                                        */
                /*****************************************************/
                } while (new_sd != -1);
            }

            /*********************************************************/
            /* This is not the listening socket, therefore an        */
            /* existing connection must be readable                  */
            /*********************************************************/

            else
            {
                // This happens evertime new data is sent by the client
                // printf("  Descriptor %d is readable\n", fds[i].fd );
                printf("  Client %d is readable\n", i );
                close_conn = false;
                /*******************************************************/
                /* Receive all incoming data on this socket            */
                /* before we loop back and call poll again.            */
                /*******************************************************/

                do
                {
                    /*****************************************************/
                    /* Receive data on this connection until the         */
                    /* recv fails with EWOULDBLOCK. If any other         */
                    /* failure occurs, we will close the                 */
                    /* connection.                                       */
                    /*****************************************************/
                    rc = read( fds[i].fd, buffer, sizeof( buffer ) );
                    buffer[rc] = 0;
                    if (rc < 0)
                    {
                        if (errno != EWOULDBLOCK)
                        {
                            // this is wheer "connection reset by peer" 
                            // is written cus it's perror
                            perror("  recv() failed");
                            close_conn = true;

                        }
                        break;
                    }

                    /*****************************************************/
                    /* Check to see if the connection has been           */
                    /* closed by the client                              */
                    /*****************************************************/
                    if ( rc == 0 )
                    {
                        printf( "  Connection closed\n" );
                        close_conn = true;
                        break;
                    }

                    /*****************************************************/
                    /* Data was received                                 */
                    /*****************************************************/
                    len = rc;
                    printf("  %d bytes received\n", len);
                    totalTrans++;
                    cout << "Buffer contents: ";
                    if( fputs(buffer, stdout) == EOF )
                    {
                        printf("\n Error : Fputs error\n");
                    }
                    cout << endl;

                    // put the read function here?



                    // sscanf messes up the buffer meant for sending, so i copy it

                    // int i;
                    // sscanf(buffer, "%d", &i);
                    // cout << "I: " << i << endl;
                    string stringInt( buffer );
                    // cout << "Buffer: " << buffer << endl;
                    // cout << "Stringint: " << stringInt << endl;

                    int nTime = std::stoi( stringInt );
                    // cout << "nTime recieved: " << nTime << endl;

                    Trans( nTime );

                    /*
                        read the number from the socket
                            if number is EOF, close connection
                        do the trans(n )

                    */
                    // at his point the server should send to the socket that the trans
                    //action is done, need the global counter for transactions done

                    // trans function
                    string totalTransString = to_string( totalTrans );
                    string tempString = "D" + totalTransString;
                    const char* stringToSend = tempString.c_str();
                    snprintf( sendBuff, sizeof( sendBuff ), stringToSend );
                    // snprintf( sendBuff, sizeof( sendBuff ), "%s", numToSend );
                    // write( listen_sd, sendBuff, strlen( sendBuff ) );




                    /*****************************************************/
                    /* Echo the data back to the client                  */
                    /*****************************************************/
                    // rc = send(fds[i].fd, buffer, len, 0);
                    rc = send(fds[i].fd, sendBuff, 7, 0);
                    if (rc < 0)
                    {
                        perror("  send() failed");
                        close_conn = true;
                        break;
                    }

                } while(true);

                /*******************************************************/
                /* If the close_conn flag was turned on, we need       */
                /* to clean up this active connection. This            */
                /* clean up process includes removing the              */
                /* descriptor.                                         */
                /*******************************************************/
                if (close_conn)
                {
                    close(fds[i].fd);
                    fds[i].fd = -1;
                    compress_array = true;
                }


            }  /* End of existing connection is readable             */
        } /* End of loop through pollable descriptors              */

        /***********************************************************/
        /* If the compress_array flag was turned on, we need       */
        /* to squeeze together the array and decrement the number  */
        /* of file descriptors. We do not need to move back the    */
        /* events and revents fields because the events will always*/
        /* be POLLIN in this case, and revents is output.          */
        /***********************************************************/
        if (compress_array)
        {
            compress_array = false;
            for (i = 0; i < nfds; i++)
            {
                if (fds[i].fd == -1)
                {
                for(j = i; j < nfds; j++)
                {
                    fds[j].fd = fds[j+1].fd;
                }
                i--;
                nfds--;
                }
            }
        }
    } while (end_server == false); /* End of serving running.    */
}


void cleanUp()
{
    /*************************************************************/
    /* Clean up all of the sockets that are open                 */
    /*************************************************************/
    for (i = 0; i < nfds; i++)
    {
        if(fds[i].fd >= 0)
        close(fds[i].fd);
    }
}


// at the end for the summary just send the entire string
int main (int argc, char *argv[])
{
    setup( argc, argv );

    serverLoop();

    cleanUp();
}

