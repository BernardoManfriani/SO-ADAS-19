#include <stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include<signal.h>

#include "../lib/socketManager.h"
#include "../lib/fileManager.h"

char *startMode;

FILE *readFd;
FILE *logFd;

int socketFd;

unsigned char data[24];
int bytesRead;

void init();
void sigTermHandler();

void readFromFile();
void writeRadarLog(FILE *, unsigned char data[]);


int main(int argc, char *argv[]){
	startMode = argv[1];

	signal(SIGTERM, sigTermHandler);
	init();

	readFromFile();

}

void init() {
	socketFd = connectClient("ffrSocket");

	if(strcmp(startMode, "NORMALE") == 0) {
 		openFile("/dev/random","r", &readFd);
	} else {
 		openFile("../data/randomARTIFICIALE.binary","r", &readFd);
	}

	openFile("../log/radar.log","w", &logFd);
}

void readFromFile() {
	while(1) {
		bytesRead = fread(data, 1, 24, readFd);
		if (bytesRead == 24) {
			writeSocket(socketFd, data);		// scrivo su socket ffrSocket
		    writeRadarLog(logFd, data);
			fflush(logFd);
		}

		sleep(2);
	}

	fclose(readFd);
	fclose(logFd);
}

void writeRadarLog(FILE *logFd, unsigned char data[]) {
	for(int i = 0; i < 24; ++i){
        fprintf(logFd, "%02X", data[i]);
    }
    fprintf(logFd, "\n");
}

void sigTermHandler(){
	fclose(readFd);
	fclose(logFd);
	exit(0);
}