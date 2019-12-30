#include <stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include "socketManager.h"

void test();
void readFromFile();
void writeLog();

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
  	printf("SENSORE ffr: modalità avvio %s\n", argv[1]);

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
	if(readFd == NULL) {
		printf("DIOHANEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEe\n");
	}

	logFd = fopen("radar.log", "w");

	int i = 0;
	while(i < 3) {
		printf("FFR DENTRO IL CICLO\n");
		bytesRead = fread(data, 1, 24, readFd);
		if (bytesRead == 24) {			
			writeSocket(socketFd, data);		// scrivo su socket ffr <--> ecu
		    fprintf(logFd, "%s", data);
			fflush(logFd);
		}

		sleep(2);
		i++;
	}

	fclose(readFd);
	fclose(logFd);
}

void test() {
	readFd = fopen("/dev/random", "r");		// look: eseguire due volte di seguito, alla seconda sembra non riesca ad aprire il file
	int i;
    unsigned char buffer[8];
    fread(buffer, 1, 8, readFd);
    fclose(readFd);

    unsigned char errValues[] = {0xA0, 0x0F, 0xB0, 0x72, 0x2F, 0xA8, 0x83, 0x59, 0xCE, 0x23};
    for(i = 0; i < 8; ++i){
        printf("%02X-", buffer[i]);
    }
    printf("\n");

    for(int i = 0; i < 10; i++) {
		for(int j = 0; j < 8; j++) {
			if(buffer[j] == errValues[i]) {
				printf("TROVATO SIMILE - ERROR %02X\n", buffer[j]);
				return ;
			}
		}
    }

	printf("BELLA\n");
}