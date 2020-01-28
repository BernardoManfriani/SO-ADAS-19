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
void manageData(char *socketData);
void brakeTillStop(int speed);
void writeLog();
int getDeceleration(char *socketData);

void lettorePipe();

int main() {
    printf("ATTUATORE bbw: attivo\n");
    pidEcu = getppid();

	openFile("brake.log", "w", &fileLog);		// apro file - può arrivare un segnale di pericolo ancora prima di aver "toccato freno"
    initPipe();

	fcntl(pipeFd[READ], F_SETFL, O_NONBLOCK);	// rende la read su pipe non bloccante

	pidWriter = fork();
	if(pidWriter == 0) {			// child process writer on brake.log file
		printf("ATTUATORE bbw: write in brake.log\n");
		close(pipeFd[WRITE]);
		writeLog();
		close(pipeFd[READ]);
		//fclose(fileLog);	--------------------------------------------------------------------------------------------
	} else {				// father process listener on socket
    	signal(SIGDANGER, dangerHandler);
		close(pipeFd[READ]);
        createServer();

        wait(NULL);		// look: mettendo questo wait mi è stato possibile creare 2 soli bbw server, senza fare un while(1)
		close(pipeFd[WRITE]);
	}

    printf("ATTUATORE bbw: TERMINO\n");
	return 0;
}

void dangerHandler() {
    //signal(SIGDANGER, dangerHandler);			// look: resettare signal ???
	fprintf(fileLog, "ARRESTO AUTO\n");
	//fflush(fileLog);
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

    for(int j = 0; j < 2; j++) {	// creo 2 BBW-SERVER
		printf("ATTUATORE-SERVER bbw-%d: wait client\n", getpid());

		clientFd = accept (serverFd, clientSockAddrPtr, &clientLen);	// bloccante
		printf("ATTUATORE-SERVER bbw-%d: accept client\n", getpid());

		if (fork () == 0) { /* Create child */
	        char data[30];

			printf("ATTUATORE-SERVER bbw-%d: wait to read something from CLIENT\n", getpid());
	 		while(readSocket(clientFd, data)) {
	        	manageData(data);
			}

			printf("ATTUATORE-SERVER bbw-%d: end to read socket\n", getpid());

	 		close (clientFd); /* Close the socket */
	 		exit (0); /* Terminate */
		} else {
			close (clientFd); /* Close the client descriptor */
		}
	} 
}

void manageData(char *socketData) {
	//printf("ENTRO IN manageData ---\n");
	char *command = strtok(strdup(socketData), " ");	
	//printf("COMMAND = '%s'\n", command);
	deltaSpeed = getDeceleration(socketData);
	//printf("DELTA SPEED = '%d'\n", deltaSpeed);

	if(strcmp(command, "PARCHEGGIO") == 0) {	//look: metto qui => non considero più i comandi che mi arrivano da ECU-CLIENT
		printf("BBW PARCHEGGIO\n");
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
			//printf("BBW-LOGGER: HO LETTO '%s'\n", socketData);
			char *command = strtok(strdup(socketData), " ");	// E' necessario passare come argomento un duplicato della stringa
			deltaSpeed = getDeceleration(socketData);

			while(deltaSpeed > 0) {
			    //printf("ATTUATORE bbw: DECREMENTO 5 => deltaSpeed = %d\n", deltaSpeed);
			    fprintf(fileLog, "%s", "DECREMENTO 5\n");
				fflush(fileLog);

				deltaSpeed = deltaSpeed - 5;
				sleep(1);
			}
		} else {
			//printf("ATTUATORE bbw: NO ACTION\n");
		    fprintf(fileLog, "%s", "NO ACTION\n");
			fflush(fileLog);

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
	char *prova = strdup(socketData);
	char *decelerazione = strtok(prova," ");			// look: prende primo comando
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
