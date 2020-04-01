/*------------------------------------------------------
    Adapted from 
    https://www.ibm.com/support/knowledgecenter/ssw_ibm_i_71/rzab6/poll.htm

    Johnas Wong
    CCID: johnas
    ID: 1529241
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
#include <unordered_map>
#include <iterator>
#include <chrono>

using namespace std;

void Trans( int n );    // Forward declarations of the provided functions without using header file
void Sleep( int n );

int    socketData, on = 1, timeout, transactionsDone = 0;
int    listen_sd = -1, new_sd = -1;
int    end_server = false, compress_array = false;
int    close_conn;
char   buffer[1025];
char   sendBuff[1025];
struct sockaddr_in   addr;
struct pollfd fds[200];
int    nfds = 1, current_size = 0, i, j;
int    totalTrans = 0;
unordered_map<string, int> clients;
bool   timeStarted = false;
auto   start_time = std::chrono::system_clock::now();
chrono::duration<double> elapsed_time;


// Print the epoch time
void printEpochTime()
{
    cout << setfill('0');
    struct timeval tv;

    // epoch time in ms
    gettimeofday(&tv,NULL);
    unsigned long long seconds = tv.tv_sec;
    unsigned long long millisecs = tv.tv_usec / 10000;

    cout << seconds << "." << setw(2) << millisecs << ": ";
    cout << setfill(' ');
}


// Set up the sockets and connections
void setup( int argc, char *argv[] )
{
    if( argc != 2 )
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
        cout << "Using port " << portNum << endl;
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
    socketData = setsockopt(listen_sd, SOL_SOCKET,  SO_REUSEADDR,
                    (char *)&on, sizeof(on));
    if (socketData < 0)
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
    socketData = ioctl(listen_sd, FIONBIO, (char *)&on);
    if (socketData < 0)
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
    addr.sin_port        = htons( portNum );
    socketData = ::bind(listen_sd, (struct sockaddr *)&addr, sizeof(addr));
    if (socketData < 0)
    {
        perror("bind() failed");
        close(listen_sd);
        exit(-1);
    }

    /*************************************************************/
    /* Set the listen back log                                   */
    /*************************************************************/
    socketData = listen(listen_sd, 32);
    if (socketData < 0)
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
    /*************************************************************/
    timeout = ( 60 * 1000 );
}


// Main loop of server, stays in here until inactivity timeout of 60 seconds passes with no connected clients
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
        socketData = poll(fds, nfds, timeout);

        /***********************************************************/
        /* Check to see if the poll call failed.                   */
        /***********************************************************/
        if (socketData < 0)
        {
            perror("  poll() failed");
            break;
        }

        /***********************************************************/
        /* Check to see if the 60 second time out expired.          */
        /***********************************************************/
        if (socketData == 0)
        {
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
                    socketData = read( fds[i].fd, buffer, sizeof( buffer ) );                    
                    buffer[socketData] = 0;
                    if (socketData < 0)
                    {
                        if (errno != EWOULDBLOCK)
                        {
                            close_conn = true;
                        }
                        break;
                    }

                    /*****************************************************/
                    /* Check to see if the connection has been           */
                    /* closed by the client                              */
                    /*****************************************************/
                    if ( socketData == 0 )
                    {
                        close_conn = true;
                        break;
                    }

                    /*****************************************************/
                    /* Data was received                                 */
                    /*****************************************************/
                    if( timeStarted == false )
                    {
                        start_time = chrono::system_clock::now();
                        timeStarted = true;
                    }

                    totalTrans++;
                    string tempBuffer = string( buffer );
                    string commandNumStringFormat = tempBuffer.substr( 0, tempBuffer.find(",") );
                    string clientName = tempBuffer.substr( tempBuffer.find(",") + 1, tempBuffer.length() );


                    unordered_map<string,int>::iterator it = clients.find( clientName );
                    // key already present in the map
                    if (it != clients.end()) {
                        it->second++;	// increment map's value for key clientName
                    } else {        // key not found
                        clients.insert(std::make_pair(clientName, 1));
                    }

                    string commandNumInt( commandNumStringFormat );
                    printEpochTime();
                    cout << "#" << setw(3) << totalTrans << " (T" << setw(3) << 
                        commandNumInt << ") from " << clientName << endl;

                    string stringInt( buffer );

                    int nTime = std::stoi( commandNumInt );

                    Trans( nTime );

                    printEpochTime();
                    cout << "#" << setw(3) << totalTrans << 
                        " (Done) from " << clientName << endl;
                    
                    elapsed_time = std::chrono::system_clock::now() 
                        - start_time;

                    string totalTransString = to_string( totalTrans );
                    const char* stringToSend = totalTransString.c_str();
                    snprintf( sendBuff, sizeof( sendBuff ), "%s", stringToSend );

                    // Send data to the client
                    socketData = send(fds[i].fd, sendBuff, 10, 0);
                    if (socketData < 0)
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


            }
        }

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
    } while (end_server == false);
}


// Close all still-open connections
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


int main (int argc, char *argv[])
{
    setup( argc, argv );

    serverLoop();

    cleanUp();

    cout << endl << "SUMMARY" << endl;
    for (auto &e: clients) {
        cout << setw(4) << e.second << " transactions from " << e.first << endl;
	}
    double execution_time = elapsed_time.count();
    float transPerSec = totalTrans / execution_time;
    cout << setw(4) << transPerSec << " transactions/sec  (" << totalTrans 
        << "/" << setprecision(2) << execution_time << ")" << endl;
}

