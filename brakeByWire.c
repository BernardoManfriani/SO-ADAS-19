#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include<signal.h>

//#include <sys/wait.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

//#include <fcntl.h>

#include "socketManager.h"
#include "fileManager.h"

#define SIGPARK SIGUSR1
#define SIGDANGER SIGUSR2

#define DEFAULT_PROTOCOL 0
#define READ 0
#define WRITE 1

pid_t pidEcu;

int status;	//pipe status
int pipeFd[2]; // pipe fd
pid_t pidWriter;
FILE *fileLog;

int deltaSpeed;

void dangerHandler();
void initPipe();
void createServer();
void manageSocketData(char *data);
void writeLog();

void lettorePipe();

int main() {
    printf("ATTUATORE bbw: attivo\n");

    pidEcu = getppid();

	openFile("brake.log", "w", &fileLog);		// apro file - può arrivare un segnale di pericolo ancora prima di aver "toccato freno"

    signal(SIGDANGER, dangerHandler);
    pause();
    printf("ATTUATORE bbw: TERMINO\n");

    /*initPipe();

    createServer();

	// fcntl(pipeFd[READ], F_SETFL, O_NONBLOCK);	//rende la read non bloccante

	pidWriter = fork();
	if(pidWriter == 0) {			// child process writer on brake.log file
		printf("ATTUATORE bbw: write in brake.log\n");
		close(pipeFd[WRITE]);
		writeLog();
		close(pipeFd[READ]);
		return 0;				// look: perchè return? 
	} else {				// father process listener on socket
		close(pipeFd[READ]); 
        createServer();
		close(pipeFd[WRITE]);
	}*/

	return 0;
}

void dangerHandler() {
    signal(SIGDANGER, dangerHandler);
	fprintf(fileLog, "%s", "ARRESTO AUTO");
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
    strcpy (serverUNIXAddress.sun_path, "bbwSocket"); /* Set name */
    unlink ("bbwSocket"); /* Remove file if it already exists */
    bind (serverFd, serverSockAddrPtr, serverLen);/*Create file*/
    listen (serverFd, 1); /* Maximum pending connection length */

    while (1) {/* Loop forever */ /* Accept a client connection */
		printf("ATTUATORE-SERVER bbw: wait client\n");

		clientFd = accept (serverFd, clientSockAddrPtr, &clientLen);	// bloccante
		printf("ATTUATORE-SERVER bbw: accept client\n");

        char data[30];

		printf("ATTUATORE-SERVER bbw: wait to read something\n");
        while(readSocket(clientFd, data)) {
            printf("ATTUATORE-SERVER bbw: leggo = '%s'\n", data);
            manageSocketData(data);
        }

		printf("ATTUATORE-SERVER bbw: end to read socket\n");

        close (clientFd); /* Close the socket */
        exit (0); /* Terminate */
    }
}

void manageSocketData(char *data) {
    //printf("ATTUATORE bbw: leggo da socket = %s -> scrivo su pipe\n", data);
	//write(pipeFd[WRITE], data, 30);     // write on pipe


	char *command = strtok(data," ");			// look: prende primo comando
	char *decelerazione = strtok(NULL," ");		// look: prende numero nel comando

	deltaSpeed = decelerazione;

	if(strcmp(command, "PARCHEGGIO") == 0) {
		printf("RALLENTO:\n");
		while(deltaSpeed > 0){
			deltaSpeed = deltaSpeed / 5;
			printf("speed: \n");
			sleep(1);
		}
		kill(pidEcu, SIGPARK);		// segnalo a ECU parcheggio terminato
	}
}

void writeLog() {
	int bytesRead;
	char socketData [30];
	while(1) {
		bytesRead = read (pipeFd[READ], socketData, 30);
		if(bytesRead != 0){
			printf ("Read %d bytes: %s\n", bytesRead, socketData);
		    fprintf(fileLog, "%s", socketData);
			fflush(fileLog);
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