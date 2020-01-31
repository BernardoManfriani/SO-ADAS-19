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

unsigned char data[16];
int bytesRead;

char *startMode;

void sigTermHandler();
void init();
void readFromFile();
void writeCamerasLog(FILE *, unsigned char data[]);


int main(int argc, char *argv[]){
	startMode = argv[1];
	signal(SIGTERM, sigTermHandler);

	init();	

	readFromFile();

}

void init() {
	socketFd = connectClient("svcSocket");

	if(strcmp(startMode, "NORMALE") == 0) {
 		openFile("/dev/urandom","r", &readFd);
	} else {
 		openFile("../data/urandomARTIFICIALE.binary","r", &readFd);
	}

 	openFile("../log/radar.log","w", &logFd);
}

void readFromFile() {
	while(1) {
		bytesRead = fread(data, 1, 16, readFd);
		if (bytesRead == 0) {
			perror("surround-view-cameras: errore in lettura");
			exit(1);
		}

		writeSocket(socketFd, data);		// scrivo su svcSocket
	  	writeCamerasLog(logFd, data);
		fflush(logFd);

		sleep(1);
	}
}

void writeCamerasLog(FILE *logFd, unsigned char data[]) {
	for(int i = 0; i < 16; ++i){
    fprintf(logFd, "%02X", data[i]);
  }
  fprintf(logFd, "\n");
}

void sigTermHandler(){
	fclose(readFd);
	fclose(logFd);
	exit(0);
}