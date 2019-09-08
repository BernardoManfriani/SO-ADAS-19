#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h> /* For AFUNIX sockets */

#define NAME "socket"

#define DEFAULT_PROTOCOL 0
#define MAX_SOCKET_NAME_SIZE 5
#define SERVER_SOCKET_NUMBER 5
#define CLIENT_SOCKET_NUMBER 3
#define MAX_DATA_SIZE 100

// char sockets_name[MAX_SOCKET_NAME_SIZE]={
// "ffrSock",
// };

int currentSpeed;

pid_t pidFcw;


void createServer();
int readFromSocket (int );
int readLines (int x, char *y);

int main() {
    currentSpeed = 0;
    printf("%s\n", "ECU avviata");

    char *argv[6]; 
    pidFcw = fork();
    if(pidFcw < 0) {
        perror("fork");
        exit(1);
    }
    if(pidFcw == 0) {  // fwc child process
        argv[0] = "./fwc";
        execv(argv[0], argv);
    } else {
        printf("%s", "creoserver\n");
        createServer();
        
    }

    return 0;
}

void createServer() {
    int serverFd, clientFd, serverLen, clientLen;
    struct sockaddr_un serverUNIXAddress; /*Server address */
    struct sockaddr_un clientUNIXAddress; /*Client address */
    struct sockaddr* serverSockAddrPtr; /*Ptr to server address*/
    struct sockaddr* clientSockAddrPtr;/*Ptr to client address*/

    /* Ignore death-of-child signals to prevent zombies */
    //signal (SIGCHLD, SIG_IGN);

    serverSockAddrPtr = (struct sockaddr*) &serverUNIXAddress;
    serverLen = sizeof (serverUNIXAddress);
    clientSockAddrPtr = (struct sockaddr*) &clientUNIXAddress;
    clientLen = sizeof (clientUNIXAddress);
    serverFd = socket (AF_UNIX, SOCK_STREAM, DEFAULT_PROTOCOL);

    serverUNIXAddress.sun_family = AF_UNIX; /* Set domain type */
    strcpy (serverUNIXAddress.sun_path, "socket"); /* Set name */
    unlink ("socket"); /* Remove file if it already exists */
    bind (serverFd, serverSockAddrPtr, serverLen);/*Create file*/
    listen (serverFd, 5); /* Maximum pending connection length */

    while (1) {/* Loop forever */ /* Accept a client connection */
        clientFd = accept (serverFd, clientSockAddrPtr, &clientLen);
        if (fork () == 0) { /* Create child to send receipe */
            while(1) {
                readFromSocket (clientFd); /* Send data */
                sleep(1);
            }
            close (clientFd); /* Close the socket */
            exit (0); /* Terminate */
        } else {
            close (clientFd); /* Close the client descriptor */
            exit (0);
        }
    }
}

int readFromSocket (int fd) {
    char str[100];
    while (readLines (fd, str)); /* Read lines until end-of-input */
    printf ("%s\n", str); /* Echo line from socket */
}

// int readLine (int fd, char *str) {
//     /* Read a single ’\0’-terminated line into str from fd */
//     int n;
//     do { /* Read characters until ’\0’ or end-of-input */
//         n = read (fd, str, 1); /* Read one character */
//     } while (n > 0 && *str++ != '\0');

//     return (n > 0); 
// }

int readLines (int fd, char *str) {
	int n;
	do {
		n = read(fd, str, 1);
	} while (n > 0 && *str++ != '\0');
    return (n > 0);
}