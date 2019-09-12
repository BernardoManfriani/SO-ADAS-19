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
FILE *readFd;
FILE *logFd;

int createClient();
void closeClient(int s);
void writeOnSocket(int s, char *data);
int connectClient(char* socketName);
void openFile(char filename[], char mode[], FILE **filePointer);
void readFile(FILE* fd,FILE* fc);

int main() {
    currentSpeed = 0;
    printf("%s\n", "frontWindShield attivo");

    // socketFd = connectClient("socket");

    openFile("frontCamera.data","r", &readFd);
  	openFile("frontCamera.log","w", &logFd);

	printf("LEGGO read %d", readFd);
	printf("LEGGO log %d", logFd);

    readFile(readFd, logFd);

    // closeClient(socketFd);

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

void writeOnSocket (int socketFd, char *data) {
    write(socketFd, data, strlen (data) + 1);
}

void readFile(FILE* fd,FILE* fc){
	char buf[10];
  	char *res;
  	while(1) {
   		res=fgets(buf, 10, fd);

   		if(res==NULL )
    		break;

    	fprintf(fc, "%s", buf);		// scrivo su file .log
    	// writeOnSocket(socketFd, buf);		// scrivo su socket fwc <--> ecu
 		sleep(1);
  	}
}

void openFile(char filename[], char mode[], FILE **filePointer) {
	*filePointer = fopen(filename, mode);
	if(*filePointer == NULL) {
		printf("Errore nell'apertura del file");
		exit(1);
	}
}