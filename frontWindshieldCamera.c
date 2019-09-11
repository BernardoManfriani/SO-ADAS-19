#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define NAME "socket"
#define DEFAULT_PROTOCOL 0

int currentSpeed;
int socketFd;

int createClient();
void closeClient(int s);
void writeShit(int s);
int connectClient(char* socketName);

int main() {
    currentSpeed = 0;
    printf("%s\n", "frontWindShield attivo");

    socketFd = connectClient("socket");

    writeShit(socketFd); /* Read the recipe */

    closeClient(socketFd);

    return 0;
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
	int result = connect(socketFd, serverSockAddrPtr, serverLen);
 	if(result < 0){
 		return result;
 	}

    printf("%s\n", "CONNESSO");
 	return socketFd;
}

void closeClient(int socketFd) {
    printf("%s\n", "CHIUDO CONNESSIONE");
    close (socketFd); /* Close the socket */
}

void writeShit (int socketFd) {
    char *s = "SHIT BRODER";

    //sleep(1);
    write(socketFd, s, strlen (s) + 1);
}