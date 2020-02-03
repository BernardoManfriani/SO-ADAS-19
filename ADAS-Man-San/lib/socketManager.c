#include<unistd.h>
#include<string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "socketManager.h"

int readSocket(int fd, char *str) {
	int n;
	do { /* Read characters until ’\0’ or end-of-input */
		n = read(fd, str, 1); /* Read one character */
	} while (n > 0 && *str++ != '\0');

	return (n > 0);
}

void writeSocket (int socketFd, char *data) {
    write(socketFd, data, strlen (data) + 1);
}

int connectClient(char *socketName){
	int socketFd, serverLen;
	struct sockaddr_un serverUNIXAddress;
	struct sockaddr* serverSockAddrPtr;

	serverSockAddrPtr = (struct sockaddr*) &serverUNIXAddress;
	serverLen = sizeof (serverUNIXAddress);
	socketFd = socket (AF_UNIX, SOCK_STREAM, DEFAULT_PROTOCOL);
	serverUNIXAddress.sun_family = AF_UNIX; /* Server domain */
	strcpy (serverUNIXAddress.sun_path, socketName);/*Server name*/

	int result;
	do {				//Loop until a connection is made with the server
		result = connect(socketFd, serverSockAddrPtr, serverLen);
		if (result == -1) sleep (1);	// Wait and then try again
	} while (result == -1);

	return socketFd;
}
