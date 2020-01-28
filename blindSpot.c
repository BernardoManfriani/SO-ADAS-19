#include <stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include<signal.h>

#include "socketManager.h"

void init();

void sigStopHandler();
void sigContHandler();

void readFromFile();
void writeRadarLog(FILE *, unsigned char data[]);

FILE *readFd;
FILE *logFd;

int socketFd;
unsigned char data[8];
int bytesRead;

char *startMode;


int main(int argc, char *argv[]){
	printf("SENSORE bs: attivo\n");

	startMode = argv[1];

	init();

	while(1) {
		signal(SIGCONT, sigContHandler);
		pause();
	}

	close(socketFd);
	fclose(readFd);
	fclose(logFd);

	return 0;
}

void init() {
	if(strcmp(startMode, "NORMALE") == 0) {
		readFd = fopen("/dev/urandom", "r");		// look: controllare se errore in apertura file
	} else {
		readFd = fopen("urandomARTIFICIALE.binary", "r");
	}

	logFd = fopen("spot.log", "w");

	socketFd = connectClient("bsSocket");
	printf("SENSORE bs: connection open\n");
}

void sigStopHandler() {
	signal(SIGCONT, sigContHandler);
	pause();
}

void sigContHandler() {
	signal(SIGSTOP, sigStopHandler);
	readFromFile();
}

void readFromFile() {
	int i = 0;
	while(1) {
		bytesRead = fread(data, 1, 8, readFd);
		if (bytesRead == 0) {
			perror("svc: errore in lettura");
			exit(1);
		}
		writeSocket(socketFd, data);		// scrivo su socket bs <--> ecu
	  	writeRadarLog(logFd, data);
		fflush(logFd);

		usleep(500000);
		i++;
	}
}

void writeRadarLog(FILE *logFd, unsigned char data[]) {
	for(int i = 0; i < 8; ++i){
        fprintf(logFd, "%02X", data[i]);
    }
    fprintf(logFd, "\n");
}
