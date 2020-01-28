#include <stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include "socketManager.h"

void test();
void readFromFile();
void writeRadarLog(FILE *, unsigned char data[]);

FILE *readFd;
FILE *logFd;

int socketFd;

unsigned char data[24];
int bytesRead;

char *startMode;


int main(int argc, char *argv[]){
	printf("SENSORE ffr: attivo\n");

	startMode = argv[1];

	socketFd = connectClient("ffrSocket");
  	printf("SENSORE ffr: connection open\n");

	readFromFile();

	close(socketFd);

	return 0;
}

void readFromFile() {
	if(strcmp(startMode, "NORMALE") == 0) {
		readFd = fopen("/dev/random", "r");		// look: controllare se errore in apertura file
	} else {
		readFd = fopen("randomARTIFICIALE.binary", "r");
	}

	logFd = fopen("radar.log", "w");

	int i = 0;
	while(1) {
		bytesRead = fread(data, 1, 24, readFd);
		if (bytesRead == 24) {
			writeSocket(socketFd, data);		// scrivo su socket ffr <--> ecu
		    writeRadarLog(logFd, data);
			fflush(logFd);
		}

		sleep(2);
		i++;
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
