#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include<signal.h>

//#include <sys/wait.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <fcntl.h>

#include "socketManager.h"
#include "fileManager.h"

#define DEFAULT_PROTOCOL 0
#define READ 0
#define WRITE 1


int status;
int pipeFd[2];
pid_t pidWriter;
FILE *fileLog;

int deltaSpeed;

void initPipe();
void createServer();
void writeLog();

int getAcceleration(char *socketData);

void lettorePipe();

int main() {
    printf("ATTUATORE tc: attivo\n");

    initPipe();

	fcntl(pipeFd[READ], F_SETFL, O_NONBLOCK);	// rende la read su pipe non bloccante

	pidWriter = fork();
	if(pidWriter == 0) {			// child process writer on brake.log file
		close(pipeFd[WRITE]);
		writeLog();
		close(pipeFd[READ]);
    	printf("ATTUATORE tc: write logger TERMINO\n");
	} else {				// father process listener on socket
		close(pipeFd[READ]);
        createServer();
		close(pipeFd[WRITE]);
   		printf("ATTUATORE tc: tc server TERMINO\n");
	}

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
    strcpy (serverUNIXAddress.sun_path, "tcSocket"); /* Set name */
    unlink ("tcSocket"); /* Remove file if it already exists */
    bind (serverFd, serverSockAddrPtr, serverLen);/*Create file*/
    listen (serverFd, 1); /* Maximum pending connection length */

    while (1) {/* Loop forever */ /* Accept a client connection */
		printf("ATTUATORE-SERVER tc: wait client\n");

		clientFd = accept (serverFd, clientSockAddrPtr, &clientLen);	// bloccante
		printf("ATTUATORE-SERVER tc: accept client\n");

        char data[30];
		printf("ATTUATORE-SERVER tc: wait to read something\n");
        while(readSocket(clientFd, data)) {
            write(pipeFd[WRITE], data, strlen(data)+1);
        }

		printf("ATTUATORE-SERVER tc: end to read socket\n");

        close (clientFd); /* Close the socket */
        exit (0); /* Terminate */
    }
}

void writeLog() {
	char socketData[30];
	openFile("throttle.log", "w", &fileLog);

	int x = 0;
	while(x <15) {							// look: per ora leggo solo 15 volte dalla pipe
		if(read(pipeFd[READ], socketData, 30) > 0){
			deltaSpeed = getAcceleration(strdup(socketData));

			while(deltaSpeed > 0) {
			    //printf("ATTUATORE tc: AUMENTO 5 => deltaSpeed = %d\n", deltaSpeed);
			    fprintf(fileLog, "%s", "AUMENTO 5\n");
				fflush(fileLog);

				deltaSpeed = deltaSpeed - 5;
				x = x+1;
				sleep(1);
			}

		} else {
			//printf("ATTUATORE tc: NO ACTION\n");
		    fprintf(fileLog, "%s", "NO ACTION\n");
			fflush(fileLog);

			x = x+1;
			sleep(1);
		}
	}

	fclose(fileLog);
}

int getAcceleration(char *socketData) {
	char *accelerazione = strtok(socketData," ");			// look: prende comando
	accelerazione = strtok(NULL," ");		// look: prende numero del comando

	return atoi(accelerazione);
}

void initPipe() {
	status = pipe(pipeFd);
	if(status != 0) {
		printf("Pipe error\n");
		exit(1);
	}
}
