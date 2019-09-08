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

void createClient();
void writeShit(int s);

int main() {
    currentSpeed = 0;
    printf("%s\n", "frontWindShield attivo");

    createClient();

    return 0;
}

void createClient() {
    int socketFd, serverLen, result;
    struct sockaddr_un serverUNIXAddress;
    struct sockaddr* serverSockAddrPtr;

    serverSockAddrPtr = (struct sockaddr*) &serverUNIXAddress;
    serverLen = sizeof (serverUNIXAddress);

    socketFd = socket (AF_UNIX, SOCK_STREAM, DEFAULT_PROTOCOL);
    serverUNIXAddress.sun_family = AF_UNIX; /* Server domain */
    strcpy (serverUNIXAddress.sun_path, "socket");/*Server name*/

    do { /* Loop until a connection is made with the server */
        result = connect (socketFd, serverSockAddrPtr, serverLen);
        if (result == -1) sleep (1); /* Wait and then try again */
    } while (result == -1);
    printf("%s\n", "CONNESSO");

    for(int i = 0; i<5; i++){
        writeShit(socketFd); /* Read the recipe */
    }
    close (socketFd); /* Close the socket */
    exit(0); /* EXIT_SUCCESS */ 
}

void writeShit (int socketFd) {
    char *s = "SHIT BRODER";

    //sleep(1);
    write(socketFd, s, strlen (s) + 1);
}