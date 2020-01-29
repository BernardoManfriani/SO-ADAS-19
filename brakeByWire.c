#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include<signal.h>

#include <sys/wait.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <fcntl.h>

#include "../lib/socketManager.h"
#include "../lib/fileManager.h"

#define SIGPARK SIGUSR1
#define SIGDANGER SIGUSR2

#define DEFAULT_PROTOCOL 0
#define READ 0
#define WRITE 1

pid_t pidEcu;

int deltaSpeed;

int status;
int pipeFd[2];
pid_t pidWriter;
FILE *fileLog;

void initPipe();
void dangerHandler();

void createServer();
void manageData(char *socketData);
void brakeTillStop(int speed);
void writeLog();
int getDeceleration(char *socketData);


int main() {
    pidEcu = getppid();

	openFile("../log/brake.log", "w", &fileLog);	// apro file - può arrivare un segnale di pericolo ancora prima di aver "toccato freno"
    initPipe();

    signal(SIGDANGER, dangerHandler);

	fcntl(pipeFd[READ], F_SETFL, O_NONBLOCK);	// rende la read su pipe non bloccante

	pidWriter = fork();
	if(pidWriter == 0) {			// child process writer on brake.log file
		close(pipeFd[WRITE]);
		writeLog();
		close(pipeFd[READ]);
	} else {				// father process listener on socket
		close(pipeFd[READ]);
        createServer();

        wait(NULL);		// look: mettendo questo wait mi è stato possibile creare 2 soli bbw server, senza fare un while(1)
		close(pipeFd[WRITE]);
	}

	return 0;
}

void dangerHandler() {
    fprintf(fileLog, "ARRESTO AUTO\n");

    fclose(fileLog);
	kill(getpid(), SIGTERM);
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

    for(int j = 0; j < 2; j++) {
		clientFd = accept (serverFd, clientSockAddrPtr, &clientLen);

		if (fork () == 0) {
	        char data[30];

	 		while(readSocket(clientFd, data)) {
	        	manageData(data);
			}

	 		close (clientFd); /* Close the socket */
	 		exit (0); /* Terminate */
		} else {
			close (clientFd); /* Close the client descriptor */
		}
	} 
}

void manageData(char *socketData) {
	char *command = strtok(strdup(socketData), " ");
	deltaSpeed = getDeceleration(socketData);

	if(strcmp(command, "PARCHEGGIO") == 0) {
		brakeTillStop(deltaSpeed);
		deltaSpeed = 0;

		kill(pidWriter, SIGTERM);
		exit(0);
	} else {
		write(pipeFd[WRITE], socketData, strlen(socketData)+1);
	}
}

void writeLog() {
	int bytesRead;
	char socketData [30];

	sleep(1);

	while(1) {
		if(read(pipeFd[READ], socketData, 30) > 0) {
			char *command = strtok(strdup(socketData), " ");	// Passo come argomento un duplicato della stringa
			deltaSpeed = getDeceleration(socketData);

			while(deltaSpeed > 0) {
			    fprintf(fileLog, "%s", "DECREMENTO 5\n");
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

void brakeTillStop(int speed) {
	printf("RALLENTO:\n");
	while(speed > 0){
		printf("speed: %d\n", speed);
		speed = speed - 5;

		sleep(1);
	}

	printf("PARCHEGGIATO\n");
	kill(pidEcu, SIGPARK);		// segnalo a ECU che ho rallentato fino a fermarmi
}

int getDeceleration(char *socketData) {
	char *prova = strdup(socketData);
	char *decelerazione = strtok(prova," ");
	decelerazione = strtok(NULL," ");

	return atoi(decelerazione);
}

void initPipe() {
	status = pipe(pipeFd);
	if(status != 0) {
		printf("Pipe error\n");
		exit(1);
	}
}
