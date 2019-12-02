#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "socketManager.h"
#include "fileManager.h"

#define DEFAULT_PROTOCOL 0
#define READ 0
#define WRITE 1

void createServer();

int main() {
    printf("ATTUATORE sbw: attivo\n");

    createServer();

	return 0;
}

void createServer() {
    int serverFd, clientFd, serverLen, clientLen;
    struct sockaddr_un serverUNIXAddress; /*Server address */
    struct sockaddr_un clientUNIXAddress; /*Client address */
    struct sockaddr* serverSockAddrPtr; /*Ptr to server address*/
    struct sockaddr* clientSockAddrPtr;/*Ptr to client address*/

    serverSockAddrPtr = (struct sockaddr*) &serverUNIXAddress;
    serverLen = sizeof (serverUNIXAddress);
    clientSockAddrPtr = (struct sockaddr*) &clientUNIXAddress;
    clientLen = sizeof (clientUNIXAddress);
    serverFd = socket (AF_UNIX, SOCK_STREAM, DEFAULT_PROTOCOL);

    serverUNIXAddress.sun_family = AF_UNIX; /* Set domain type */
    strcpy (serverUNIXAddress.sun_path, "sbwSocket"); /* Set name */
    unlink ("sbwSocket"); /* Remove file if it already exists */
    bind (serverFd, serverSockAddrPtr, serverLen);/*Create file*/
    listen (serverFd, 1); /* Maximum pending connection length */

    while (1) {/* Loop forever */ /* Accept a client connection */
		printf("ATTUATORE-SERVER sbw: wait client\n");

		clientFd = accept (serverFd, clientSockAddrPtr, &clientLen);	// bloccante
		printf("ATTUATORE-SERVER sbw: accept client\n");

        char data[30];

		printf("ATTUATORE-SERVER sbw: wait to read something\n");
        while(readSocket(clientFd, data)) {
            //manageSocketData(data);
            printf("ATTUATORE-SERVER sbw: leggo = '%s'\n", data);
        }

		printf("ATTUATORE-SERVER sbw: end to read socket\n");

        close (clientFd); /* Close the socket */
        exit (0); /* Terminate */
    }
}