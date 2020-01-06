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

#define SIGRESETPARK SIGUSR2

int socketFd;

FILE *readFd;
FILE *logP; //assist.log descriptor

char *startMode;

void readSend();

int main(int argc, char *argv[]){

	printf("SENSORE pa: attivo\n");

	startMode = argv[1];

	socketFd = connectClient("paSocket");
  	printf("SENSORE pa: connection open\n");

	readSend();

	return 0;

}

void readSend(){
	signal(SIGRESETPARK, readSend);

	if(strcmp(startMode, "NORMALE") == 0) {
		readFd = fopen("/dev/urandom", "r");		// look: controllare se errore in apertura file
	} else {
		readFd = fopen("urandomARTIFICIALE.binary", "r");
	}
	openFile("assist.log","w", &logP);

	unsigned char buffer[4];
	int i=0;
	while (i < 30){
		sleep(1);
		printf("Sto inviando alla ecu\n ");
		fread(buffer, 1, 4, readFd);
		//read(readFd, &buffer, 4);

    for(int j=0; j < 4; j++){
			fprintf(logP , "%02X", buffer[j]);
		}
		fprintf(logP , "\n");

		writeSocket(socketFd, buffer);

		//buffer now contains the random data
		i++;
	}

	fclose(readFd);
	fclose(logP);
}
