#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "socketManager.h"
#include "fileManager.h"


//void sendEcu(unsigned char buffer);

int socketFd;

FILE * logB;

unsigned char generateReadRand(unsigned char buffer);

void readSend();

int main(){
	printf("SENSORE bs: attivo\n");

	socketFd = connectClient("bsSocket");
	printf("SENSORE bs: connection open\n");

	readSend();

	return 0;
}


unsigned char generateReadRand(unsigned char buffer){
	int fd = open("/dev/urandom", O_RDONLY);
	read(fd, &buffer, 8);
	//buffer now contains the random data
	close(fd);
	return buffer;
}

void readSend(){

	openFile("spot.log","w", &logB);

	unsigned char buffer[8];
	int i=0;

	while (i < 10){
		sleep(0.5);
		printf("Sto inviando alla ecu ");

		for(int j=0; j<8; j++){
			buffer[j] = generateReadRand(buffer[j]);
			printf("%.2x",buffer[j]);
			fprintf(logB , "%.2x", buffer[j]);
		}
		writeSocket(socketFd, buffer);
		fprintf(logB, "%s", "\n" );
		printf("\n");
		i++;
	}
}
