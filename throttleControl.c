#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include<signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <fcntl.h>

#include "socketManager.h"
#include "fileManager.h"

#define DEFAULT_PROTOCOL 0
#define READ 0
#define WRITE 1


int status;	//pipe status
int pipeFd[2]; // pipe fd
pid_t pidWriter;
FILE *fileLog;

int deltaSpeed;

void initPipe();
void createServer();
void manageSocketData(char *data);
void writeLog();

void lettorePipe();

int main() {
    printf("ATTUATORE tc: attivo\n");

    initPipe();

	// fcntl(pipeFd[READ], F_SETFL, O_NONBLOCK);	//rende la read non bloccante

	pidWriter = fork();
	if(pidWriter == 0) {			// child process writer on throttle.log file
		printf("ATTUATORE tc: write in throttle.log\n");
		close(pipeFd[WRITE]);
		writeLog();
		close(pipeFd[READ]);
		return 0;				// look: perchè return? 
	} else {				// father process listener on socket
		close(pipeFd[READ]); 
        createServer();
		close(pipeFd[WRITE]);
	}
	exit(0);
}

void createServer() {
    int serverFd, clientFd, serverLen, clientLen;
    struct sockaddr_un serverUNIXAddress; /*Server address */
    struct sockaddr_un clientUNIXAddress; /*Client address */
    struct sockaddr* serverSockAddrPtr; /*Ptr to server address*/
    struct sockaddr* clientSockAddrPtr;/*Ptr to client address*/

    /* Ignore death-of-child signals to prevent zombies */
    signal (SIGCHLD, SIG_IGN);

    serverSockAddrPtr = (struct sockaddr*) &serverUNIXAddress;
    serverLen = sizeof (serverUNIXAddress);
    clientSockAddrPtr = (struct sockaddr*) &clientUNIXAddress;
    clientLen = sizeof (clientUNIXAddress);
    serverFd = socket (AF_UNIX, SOCK_STREAM, DEFAULT_PROTOCOL);

    serverUNIXAddress.sun_family = AF_UNIX; /* Set domain type */
    strcpy (serverUNIXAddress.sun_path, "tcSocket"); /* Set name */
    unlink ("tcSocket"); /* Remove file if it already exists */
    bind (serverFd, serverSockAddrPtr, serverLen);/*Create file*/
    listen (serverFd, 5); /* Maximum pending connection length */

    while (1) {/* Loop forever */ /* Accept a client connection */
		printf("ATTUATORE tc: SERVER-wait client\n");

		clientFd = accept (serverFd, clientSockAddrPtr, &clientLen);	// bloccante
		printf("ATTUATORE tc: SERVER-accept client\n");

        char data[30];

        if(fork () == 0) { /* Create child read ECU client */
			printf("ATTUATORE tc: SERVER-wait to read something\n");
            while(readSocket(clientFd, data)) {
                manageSocketData(data);
            }

			printf("ATTUATORE tc: SERVER-end to read socket\n");

            close (clientFd); /* Close the socket */
            exit (0); /* Terminate */
        } else {
            close (clientFd); /* Close the client descriptor */
            exit (0);				// look: e' giusto fare un exit?
        }
    }
}

void manageSocketData(char *data) {
    printf("ATTUATORE tc: leggo da socket = %s -> scrivo su pipe\n", data);
	write(pipeFd[WRITE], data, 30);     // write on pipe
}

void writeLog() {
	openFile("throttle.log", "w", &fileLog);

	int bytesRead;
	char socketData [30];
	// char *socketData;		// ----------------- PERCHE CON QUESTO NON FUNZIONA -----------------	//
	while(1) {
		bytesRead = read (pipeFd[READ], socketData, 30);
		if(bytesRead != 0){
			printf ("Read %d bytes: %s\n", bytesRead, socketData);
		    fprintf(fileLog, "%s", socketData);
			fflush(fileLog);		// SE COMMENTO NON SCRIVE SU FILE-- CAZZO PERCHEEEEE TROIO -- (non libera buffer, e quindi?!)
		}
		sleep(1);
	}
	
}



void initPipe() {
	status = pipe(pipeFd);
	if(status != 0) {
		printf("Pipe error\n");
		exit(1);
	}
}