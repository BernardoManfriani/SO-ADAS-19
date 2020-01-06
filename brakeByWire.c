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

#define SIGPARK SIGUSR1
#define SIGDANGER SIGUSR2

#define DEFAULT_PROTOCOL 0
#define READ 0
#define WRITE 1

pid_t pidEcu;

int status;
int pipeFd[2];
pid_t pidWriter;
FILE *fileLog;

int deltaSpeed;

void dangerHandler();
void initPipe();
void createServer();
void brakeTillStop(int speed);
void writeLog();
int getDeceleration(char *socketData);

void lettorePipe();

int main() {
    printf("ATTUATORE bbw: attivo\n");
    pidEcu = getppid();

    signal(SIGDANGER, dangerHandler);
    initPipe();

	fcntl(pipeFd[READ], F_SETFL, O_NONBLOCK);	// rende la read su pipe non bloccante

	openFile("brake.log", "w", &fileLog);		// apro file - può arrivare un segnale di pericolo ancora prima di aver "toccato freno"

	pidWriter = fork();
	if(pidWriter == 0) {			// child process writer on brake.log file
		printf("ATTUATORE bbw: write in brake.log\n");
		close(pipeFd[WRITE]);
		writeLog();
		close(pipeFd[READ]);
		fclose(fileLog);
	} else {				// father process listener on socket
		close(pipeFd[READ]);
        createServer();
		close(pipeFd[WRITE]);
	}

    printf("ATTUATORE bbw: TERMINO\n");
	return 0;
}

void dangerHandler() {
    //signal(SIGDANGER, dangerHandler);			// look: resettare signal ???
	fprintf(fileLog, "%s\n", "ARRESTO AUTO");
	kill(getpid(), SIGTERM);		// uccide processo
	// look: uccidere anche processo writer !?
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
    listen (serverFd, 2); /* Maximum pending connection length */

    for(int j = 0; j < 1; j++) {		// look: con questo for forzo l'esistenza di due soli BBW-SERVER
		printf("ATTUATORE-SERVER bbw: wait client\n");

		clientFd = accept (serverFd, clientSockAddrPtr, &clientLen);	// bloccante
		printf("ATTUATORE-SERVER bbw: accept client\n");

		if (fork () == 0) { /* Create child */
	        char data[30];
			printf("ATTUATORE-SERVER bbw: wait to read something\n");
	 		while(readSocket(clientFd, data)) {
				printf("--------- BBW server READ SOMETHING\n");
	        	write(pipeFd[WRITE], data, strlen(data)+1);
			}

			printf("ATTUATORE-SERVER bbw: end to read socket\n");

	 		close (clientFd); /* Close the socket */
	 		exit (0); /* Terminate */
		} else {
			close (clientFd); /* Close the client descriptor */
		}
	}    
}

void writeLog() {
	int bytesRead;
	char socketData [30];

	int x = 0;
	while(1) {							// look: per ora leggo solo 20 volte dalla pipe
		if(read(pipeFd[READ], socketData, 30) > 0) {
			printf("--------- BBW WRITE LOG => '%s'\n", socketData);
			//char *command = strtok(strdup(socketData), " ");	// look: è necessario passare come argomento un duplicato della stringa
			//deltaSpeed = getDeceleration(strdup(socketData));

			/*if(strcmp(command, "PARCHEGGIO") == 0) {
				printf("BBW ------ STO PARCHEGGIANDO");
				brakeTillStop(deltaSpeed);
				deltaSpeed = 0;
			}*/

			while(deltaSpeed > 0) {
			    //printf("ATTUATORE bbw: DECREMENTO 5 => deltaSpeed = %d\n", deltaSpeed);
			    fprintf(fileLog, "%s", "DECREMENTO 5\n");
				fflush(fileLog);

				deltaSpeed = deltaSpeed - 5;
				x = x+1;
				sleep(1);
			}
		} else {
			//printf("ATTUATORE bbw: NO ACTION\n");
		    fprintf(fileLog, "%s", "NO ACTION\n");
			fflush(fileLog);

			x = x+1;
			sleep(1);
		}
	}
}

void brakeTillStop(int speed) {
	printf("RALLENTO:\n");
	while(speed > 0){
		speed = speed - 5;
		printf("speed: %d\n", speed);
		sleep(1);
	}
	printf("PARCHEGGIATO\n");
	kill(pidEcu, SIGPARK);		// segnalo a ECU che ho rallentato fino a fermarmi
}

int getDeceleration(char *socketData) {
	char *decelerazione = strtok(socketData," ");			// look: prende primo comando
	decelerazione = strtok(NULL," ");		// look: prende numero nel comando

	return atoi(decelerazione);
}

void initPipe() {
	status = pipe(pipeFd);
	if(status != 0) {
		printf("Pipe error\n");
		exit(1);
	}
}
