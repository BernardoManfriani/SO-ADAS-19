#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include <fcntl.h>

#include<signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "../lib/socketManager.h"
#include "../lib/fileManager.h"

#define SIGRESETPARK SIGINT
#define PARKING_TIME 30

int socketFd;

pid_t pidEcu;

FILE *readFd;
FILE *logFd;		//assist.log descriptor

char *startMode;
int resetPark = 0;

void init();
void readSend();

void sigTermHandler();
void resetParkHandler();

int main(int argc, char *argv[]){
	pidEcu = getppid();
	startMode = argv[1];
	
	signal(SIGTERM, sigTermHandler);

	init();		// connessione socket + apertura file

	readSend();
    kill(pidEcu, SIGUSR1); 	// comunico ad ECU che ha finito

    pause();	// mi assicuro di terminare il processo con il relativo handler
}

void readSend(){
	signal(SIGRESETPARK, resetParkHandler);
	unsigned char buffer[4];

	for(int i = 0; i < PARKING_TIME; i++){
		if(resetPark) {
			resetPark = 0;
			i = 0;
		}

		fread(buffer, 1, 4, readFd);
		writeSocket(socketFd, buffer);

	    for(int j=0; j < 4; j++){
			fprintf(logFd, "%02X", buffer[j]);
		}
		fprintf(logFd, "\n");

		sleep(1);
	}
}

void init() {
	socketFd = connectClient("paSocket");

	if(strcmp(startMode, "NORMALE") == 0) {
 		openFile("/dev/urandom","r", &readFd);
	} else {
 		openFile("../data/urandomARTIFICIALE.binary","r", &readFd);
	}
	openFile("../log/assist.log","w", &logFd);
}

void resetParkHandler() {
	signal(SIGRESETPARK, resetParkHandler);
	printf("---------------RESETTO-----------------\n");
	resetPark = 1;
}

void sigTermHandler(){
	fclose(readFd);
	fclose(logFd);
	exit(0);
}