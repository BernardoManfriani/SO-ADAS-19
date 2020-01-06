#include <stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include "socketManager.h"

void test();
void readFromFile();
void writeCamerasLog(FILE *, unsigned char data[]);

FILE *readFd;
FILE *logFd;

int socketFd;

unsigned char data[16];
int bytesRead;

char *startMode;


int main(int argc, char *argv[]){
	printf("SENSORE svc: attivo\n");

	startMode = argv[1];

	socketFd = connectClient("svcSocket");
  	printf("SENSORE svc: connection open\n");
  	printf("SENSORE svc: modalità avvio %s\n", argv[1]);

	readFromFile();

	close(socketFd);

	return 0;
}

void readFromFile() {
	if(strcmp(startMode, "NORMALE") == 0) {
		readFd = fopen("/dev/urandom", "r");		// look: controllare se errore in apertura file
	} else {
		readFd = fopen("urandomARTIFICIALE.binary", "r");
	}

	logFd = fopen("radar.log", "w");

	int i = 0;
	while(i < 5) {
		bytesRead = fread(data, 1, 16, readFd);
		if (bytesRead == 0) {
			perror("svc: errore in lettura");
			exit(1);
		}
		writeSocket(socketFd, data);		// scrivo su socket svc <--> ecu
	  writeCamerasLog(logFd, data);
		fflush(logFd);

		sleep(1);
		i++;
	}

	fclose(readFd);
	fclose(logFd);
}

void writeCamerasLog(FILE *logFd, unsigned char data[]) {
	for(int i = 0; i < 16; ++i){
    fprintf(logFd, "%02X", data[i]);
  }
  fprintf(logFd, "\n");
}
