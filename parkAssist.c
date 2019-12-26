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


#include "socketManager.h"
#include "fileManager.h"


FILE * logP; //assist.log descriptor

void sendEcu(unsigned char buffer);

unsigned char generateReadRand(unsigned char buffer);

void readSend();

int main(){

		printf("SENSORE pa: attivo\n");

		readSend();

		return 0;

}

unsigned char generateReadRand(unsigned char buffer){

	int fd = open("/dev/urandom", O_RDONLY);
	read(fd, &buffer, 4);
	//buffer now contains the random data
	close(fd);
	return buffer;
}

/*void sendEcu(unsigned char buffer){
	printf("Sto inviando alla ecu %.2x \n", buffer);
}
*/

void readSend(){

	openFile("assist.log","w", &logP);

	unsigned char buffer[4];
	int i=0;

	while (i < 10){
		sleep(1);
		printf("Sto inviando alla ecu ");

		for(int j=0; j<4; j++){
			buffer[j] = generateReadRand(buffer[j]);
			printf("%.2x",buffer[j]);
			fprintf(logP , "%.2x", buffer[j]);
		}
		fprintf(logP, "%s", "\n" );
		printf("\n");
		//printf("%02X ", buffer);

		i++;
	}
}
