/* From: www.thegeekstuff.com/2011/12/c-socket-programming
    https://stackoverflow.com/questions/10150468/how-to-redirect-cin-and-cout-to-files
        Nawaz file redirection
*/


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
#include <string>
#include <limits.h>
#include <sys/time.h>


#include <fstream>

using namespace std;

#define HOST_NAME_MAX 50        // don't need this, linux has

void Trans( int n );    // Forward declarations of the provided functions without using header file
void Sleep( int n );

/*
    Made these globals as to have a cleaner main function
*/

int sockfd, n, portNum, ipAddressInt;
bool commandIsSleep = false;
char recvBuff[1024];
char sendBuff[1025];

struct sockaddr_in serv_addr;
char* ipAddressConstChar;


string logFileToWriteTo()
{
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);       // should get naem of computer
    string hostPID = to_string( getpid() );
    string hostnameStringFormat( hostname );

    // cout << "Computer: " << hostnameStringFormat << endl;
    // cout << "PID: " << hostPID << endl;

    string logFileName = hostnameStringFormat + "." + hostPID + ".log";
    return logFileName;
}


void printStartingInfoToLogFile()
{
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);       // should get naem of computer
    string hostPID = to_string( getpid() );
    string hostnameStringFormat( hostname );

    cout << "Using port " << portNum << endl;
    cout << "Using server address " << ipAddressConstChar << endl;;
    cout << "Host " << hostnameStringFormat << "." << hostPID << endl;
}


void setup( int argc, char *argv[] )
{
    /*
    sockfd is the socket, the door
    */
    sockfd = 0;
    n = 0;


    if( argc != 3 ) // put input error here too
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
        exit( EXIT_FAILURE );
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
        exit( EXIT_FAILURE );
    } 

    // if it fails to connect
    /*
    ip address + port number are bundled in to the struct and a call to function
    connect() is made, tries to connect this socket with struct socket
    */
    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       printf("\n Error : Connect Failed \n");
       exit( EXIT_FAILURE );
    } 
}


void printEpochTime()
{
    struct timeval tv;

    // epoch time in ms
    gettimeofday(&tv,NULL);
    unsigned long long seconds = tv.tv_sec;
    unsigned long long millisecs = tv.tv_usec / 10000;
    cout << seconds << "." << millisecs;
}


void splitInput( string inputCommand )
{
    // only supposed to be T50 or S100 inputs
    int inputSize = inputCommand.length(); 
  
    char tOrS = inputCommand.at(0);
    string commandNum = inputCommand.erase( 0, 1 );
    int nTime = std::stoi( commandNum );
    const char* numToSend = commandNum.c_str();
    // cout << nTime << " " << numToSend << endl;

    if( tOrS == 'S' ) 
    {
        cout << "Sleeping" << nTime <<  endl;
        commandIsSleep = true;
        Sleep( nTime );
        // cout << "after sleep" << endl;
    } else if( tOrS == 'T' ) {
        // send the T<N> command
        commandIsSleep = false;

        time_t result = time(nullptr);
        // float result = std::time(nullptr);
        printEpochTime();
        std::cout << " seconds since the Epoch\n";

        // cout << "Printf: ";
        // printf("%02l", result);
        // printf("%.2f", result);
        // cout << endl;

        snprintf( sendBuff, sizeof( sendBuff ), "%s", numToSend );
        write( sockfd, sendBuff, strlen( sendBuff ) );
    }
}


void clientLoop( string line ) 
{
    splitInput( line );


    /*
    if it doesn't read from the socket properly
    read does not output, returns an int of number of bytes readf
    read: ( file descripter, into buffer, number of bytes to read )
    On success, the number of bytes read is returned (zero indicates end
    of file), and the file position is advanced by this number.
    i ouputted n, it was 26. so reads all byes at once.

    n = the nunber of bytes read, and if it's 0 or less it doens' tdo 
    anything
    */

    // cout << "Client before read" << endl;

    if( commandIsSleep == false )
    {
        // IF IT"S A SLEEP IT GETS STUCK HERE
        if( ( n = read( sockfd, recvBuff, sizeof( recvBuff ) - 1 ) ) > 0 )
        {
            // cout << n << endl;
            // this sets the end of whatever was read into the buffer to zero
            /*
            if buffer reads in a, b and c, n = 3.
            so buffer[3] == 0;
            zero is the terminating character, sending info that this is the end
            sets end of string
            */
            recvBuff[n] = 0;
            // cout << "Gotten from socket: ";
            // prints everything inside the bufffer
            // if( fputs(recvBuff, stdout) == EOF )
            // {
            //     printf("\n Error : Fputs error\n");
            // }
            cout << "REC BUFF: " << recvBuff << endl;
        } 
        // cout << "After read" << endl;

        if( n < 0 )
        {
            printf("\n Read error \n");
        } 
    }
}

/*
Clients have to send outputs to log file, server doesn't have to
*/
int main(int argc, char *argv[])
{
    // logFileToWriteTo();

    std::ofstream out( logFileToWriteTo() );
    std::streambuf *coutbuf = std::cout.rdbuf(); //save old buf
    std::cout.rdbuf(out.rdbuf()); //redirect std::cout to out.txt!
    // const char* tempFileName = logFileToWriteTo().c_str();
    // freopen( tempFileName, "w", stdout);    // redirect stdout too for fputs

    setup( argc, argv );
    printStartingInfoToLogFile();

    // i should def put a input check
    string line;
    while ( cin >> line )
    {
        // cout << line << endl;
        clientLoop( line );
    }

    std::cout.rdbuf(coutbuf); //reset to standard output again
    // freclose( tempFileName, "w", stdout);

    return 0;

}