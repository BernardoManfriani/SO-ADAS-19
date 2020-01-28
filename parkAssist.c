#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include <fcntl.h>

#include<signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "socketManager.h"
#include "fileManager.h"

#define SIGENDPARK SIGUSR1
#define SIGRESETPARK SIGUSR2

#define PARKING_TIME 10

int socketFd;

pid_t pidEcu;

FILE *readFd;
FILE *logP;		//assist.log descriptor

char *startMode;
int resetPark = 0;

void init();
void resetParkHandler();
void readSend();

int main(int argc, char *argv[]){
	printf("SENSORE pa: attivo\n");
	pidEcu = getppid();

	startMode = argv[1];

	init();		// connessione socket + apertura file

	signal(SIGRESETPARK, resetParkHandler);
	readSend();

    kill(pidEcu, SIGENDPARK); 	// COMUNICA alla ECU che ha finito

	fclose(readFd);
	fclose(logP);

	return 0;
}

void readSend(){
	unsigned char buffer[4];

	for(int i = 0; i < PARKING_TIME; i++){
		if(resetPark) {
			resetPark = 1;
			i = 0;
			printf("RESET PARK HANDLER METTE i = %d\n", i);
		}

		fread(buffer, 1, 4, readFd);
		writeSocket(socketFd, buffer);

	    for(int j=0; j < 4; j++){
			fprintf(logP, "%02X", buffer[j]);
		}
		fprintf(logP, "\n");

		sleep(1);
	}
}

void init() {
	socketFd = connectClient("paSocket");
  	printf("SENSORE pa: connection open\n");

	if(strcmp(startMode, "NORMALE") == 0) {			// apertura file lettura/scrittura
		readFd = fopen("/dev/urandom", "r");
	} else {
		readFd = fopen("urandomARTIFICIALE.binary", "r");
	}
	openFile("assist.log","w", &logP);
}

void resetParkHandler() {
	printf("ARRIVA LA SIGNAL ?!?!? --- \n");
	signal(SIGRESETPARK, resetParkHandler);
	resetPark = 1;
}