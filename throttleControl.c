#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include<signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <fcntl.h>

#include "../lib/socketManager.h"
#include "../lib/fileManager.h"

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
void sigTermHandler();


int main() {
    initPipe();

	fcntl(pipeFd[READ], F_SETFL, O_NONBLOCK);	// rende la read su pipe non bloccante

	pidWriter = fork();
	if(pidWriter == 0) {			// child process writer on brake.log file
		close(pipeFd[WRITE]);
		writeLog();
		close(pipeFd[READ]);
	} else {				// father process listener on socket
		signal(SIGTERM, sigTermHandler);

		close(pipeFd[READ]);
        createServer();
		close(pipeFd[WRITE]);
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
		clientFd = accept (serverFd, clientSockAddrPtr, &clientLen);
		printf("CLIENT-SERVER: tcSocket connected\n");

        char data[30];
        while(readSocket(clientFd, data)) {
            write(pipeFd[WRITE], data, strlen(data)+1);
        }

		printf("ATTUATORE-SERVER tc: close close tcSocket\n");
        close (clientFd); /* Close the socket */

        exit (0); /* Terminate */
    }
}

void writeLog() {
	openFile("../log/throttle.log", "w", &fileLog);

	char socketData[30];
	while(1) {
		if(read(pipeFd[READ], socketData, 30) > 0){
			deltaSpeed = getAcceleration(strdup(socketData));

			while(deltaSpeed > 0) {
			    fprintf(fileLog, "%s", "AUMENTO 5\n");
				fflush(fileLog);

				deltaSpeed = deltaSpeed - 5;
				sleep(1);
			}

		} else {
		    fprintf(fileLog, "%s", "NO ACTION\n");
			fflush(fileLog);

			sleep(1);
		}
	}

}

int getAcceleration(char *socketData) {
	char *accelerazione = strtok(socketData," ");		// get comando
	accelerazione = strtok(NULL," ");		// get numero comando

	return atoi(accelerazione);
}

void initPipe() {
	status = pipe(pipeFd);
	if(status != 0) {
		printf("Pipe error\n");
		exit(1);
	}
}

void sigTermHandler() {
	kill(pidWriter, SIGTERM);
	fclose(fileLog);
	exit(0);
}