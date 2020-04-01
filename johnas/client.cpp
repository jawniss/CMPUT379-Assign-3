/*------------------------------------------------------
    From: www.thegeekstuff.com/2011/12/c-socket-programming
    https://stackoverflow.com/questions/10150468/how-to-redirect-cin-and-cout-to-files
        Nawaz file redirection

    Johnas Wong
    CCID: johnas
    ID: 1529241
------------------------------------------------------*/

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
#include <string>
#include <limits.h>
#include <sys/time.h>
#include <iomanip>
#include <fstream>

using namespace std;

void Trans( int n );    // Forward declarations of the provided functions without using header file
void Sleep( int n );

/*
    Made these globals as to have a cleaner main function
*/
int sockfd, n, portNum, ipAddressInt, totalTrans = 0;
bool commandIsSleep = false;
char recvBuff[1024];
char sendBuff[1025];

string hostname;
string hostPID;
const char* hostnameToSend;

struct sockaddr_in serv_addr;
char* ipAddressConstChar;

#define HOST_NAME_MAX 64


// Create the name of the log file
string logFileToWriteTo()
{
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    hostPID = to_string( getpid() );
    string hostnameStringFormat( hostname );
    string logFileName = hostnameStringFormat + "." + hostPID + ".log";
    return logFileName;
}


// Get name of machine
string getHostName()
{
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    string hostnameStringFormat( hostname );
    return hostnameStringFormat;
}


// Output starting info to logfile
void printStartingInfoToLogFile()
{
    hostname = getHostName();
    cout << "Using port " << portNum << endl;
    cout << "Using server address " << ipAddressConstChar << endl;;
    cout << "Host " << hostname << "." << hostPID << endl;
}


// Set up the socket connections
void setup( int argc, char *argv[] )
{
    /*
    sockfd is the socket, the door
    */
    sockfd = 0;
    n = 0;


    if( argc != 3 )
    {
        printf("\n Usage: %s <ip of server> \n",argv[0]);
        cout << "Invalid input arguments, Exitting" << endl;
        exit( EXIT_FAILURE );
    } 

    portNum = stoi( argv[1] );

    if( portNum < 5000 or portNum > 64000 )
    {
        cout << "Port number must be between 5000 and 64,000" << endl;
        exit( EXIT_FAILURE );
    }

    ipAddressInt = stoi( argv[2] );
    ipAddressConstChar = argv[2];

    hostname = getHostName();
    hostnameToSend = hostname.c_str();

    memset( recvBuff, '0', sizeof( recvBuff ) );
    memset( sendBuff, '0', sizeof( sendBuff ) );

    if( ( sockfd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
    {
        printf( "\n Error : Could not create socket \n" );
        exit( EXIT_FAILURE );
    } 

    memset( &serv_addr, '0', sizeof( serv_addr ) ); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons( portNum ); 

    if( inet_pton( AF_INET, ipAddressConstChar, &serv_addr.sin_addr ) <= 0 )
    {
        printf("\n inet_pton error occured\n");
        exit( EXIT_FAILURE );
    } 

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       printf("\n Error : Connect Failed \n");
       exit( EXIT_FAILURE );
    } 
}


// Output the epoch time
void printEpochTime()
{
    // When printing epoch time, want seconds.10 not seconds.1
    cout << setfill('0');

    struct timeval tv;

    // epoch time in ms
    gettimeofday(&tv,NULL);
    unsigned long long seconds = tv.tv_sec;
    unsigned long long millisecs = tv.tv_usec / 10000;

    cout << seconds << "." << setw(2) << millisecs << ": ";
    cout << setfill(' ');
}


// Process the input
void splitInput( string inputCommand )
{
    char tOrS = inputCommand.at(0);
    string commandNum = inputCommand.erase( 0, 1 );
    int nTime = std::stoi( commandNum );

    if( tOrS == 'S' ) 
    {
        commandIsSleep = true;
        cout << "Sleep " << nTime << " units" << endl;;
        Sleep( nTime );
    } else if( tOrS == 'T' ) {
        commandIsSleep = false;
        printEpochTime();
        cout << "Send (" << tOrS << setw(3) << nTime << ")" << endl;
        string stringToSend = commandNum + "," + hostname + "." + hostPID;
        const char* stuffToSend = stringToSend.c_str();

        snprintf( sendBuff, sizeof( sendBuff ), "%s", stuffToSend );
        write( sockfd, sendBuff, strlen( sendBuff ) );
        totalTrans++;
    }
}


// Loop of the client to keep sending data
void clientLoop( string line ) 
{
    splitInput( line );

    if( commandIsSleep == false )
    {
        if( ( n = read( sockfd, recvBuff, sizeof( recvBuff ) - 1 ) ) > 0 )
        {
            // Set terminating character at end of buffer data
            recvBuff[n] = 0;
            printEpochTime();
            cout << "Recv (D" << setw(3) << recvBuff << ")" << endl;
        } 

        if( n < 0 )
        {
            printf("\n Read error \n");
        } 
    }
}


int main(int argc, char *argv[])
{
    std::ofstream out( logFileToWriteTo() );
    std::streambuf *coutbuf = std::cout.rdbuf(); //save old buf
    std::cout.rdbuf(out.rdbuf()); //redirect std::cout to output file

    setup( argc, argv );
    printStartingInfoToLogFile();

    string line;
    while ( cin >> line )
    {
        clientLoop( line );
    }

    cout << "Sent " << totalTrans << " transactions" << endl;

    std::cout.rdbuf(coutbuf);

    return 0;
}