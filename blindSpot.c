#include <stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include<signal.h>

#include "../lib/socketManager.h"
#include "../lib/fileManager.h"

FILE *readFd;
FILE *logFd;

int socketFd;
unsigned char data[8];
int bytesRead;

char *startMode;

void init();

void sigTermHandler();
void sigStopHandler();
void sigContHandler();

void readFromFile();
void writeRadarLog(FILE *, unsigned char data[]);


int main(int argc, char *argv[]){
	startMode = argv[1];

	signal(SIGTERM, sigTermHandler);
	init();

	while(1) {
		signal(SIGCONT, sigContHandler);
		pause();
	}

}

void init() {
	socketFd = connectClient("bsSocket");

	if(strcmp(startMode, "NORMALE") == 0) {
 		openFile("/dev/urandom","r", &readFd);
	} else {
 		openFile("../data/urandomARTIFICIALE.binary","r", &readFd);
	}

	openFile("../log/spot.log","w", &logFd);
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
	while(1) {
		bytesRead = fread(data, 1, 8, readFd);
		if (bytesRead == 0) {
			perror("blind-spot: errore in lettura");
			exit(1);
		}
		writeSocket(socketFd, data);		// scrivo su socket bsSocket
	  	writeRadarLog(logFd, data);
		fflush(logFd);

		usleep(500000);
	}
}

void writeRadarLog(FILE *logFd, unsigned char data[]) {
	for(int i = 0; i < 8; ++i){
        fprintf(logFd, "%02X", data[i]);
    }
    fprintf(logFd, "\n");
}

void sigTermHandler(){
	fclose(readFd);
	fclose(logFd);
	exit(0);
}