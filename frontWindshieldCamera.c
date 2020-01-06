#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "socketManager.h"
#include "fileManager.h"

int currentSpeed;
int socketFd;
FILE *readFd;
FILE *logFd;

void closeClient(int s);

void readFile(FILE *fd,FILE *fc);

int main() {
  currentSpeed = 0;
  printf("SENSORE fwc: attivo\n");

  socketFd = connectClient("fwcSocket");
  printf("SENSORE fwc: connection open\n");

  openFile("frontCamera.data","r", &readFd);      // look: PROSSIMO PASSO - APRIRE FILE .data 1.LEGGERE ROBA E SCRIVERE SU LOG
  openFile("camera.log","w", &logFd);

  readFile(readFd, logFd);            // leggo file -> 1.Scrivo su fwcSocket 2.Scrivo su .log

  closeClient(socketFd);
  printf("%s\n", "SENSORE fwc: connection close");

  return 0;
}

void closeClient(int socketFd) {
    close(socketFd); /* Close the socket */
}

void readFile(FILE *fd,FILE *fc){
  char buf[20];               // look: 20 va è sufficiente?
  char *res;
  int i = 0;
  while(i < 10) {
  	res=fgets(buf, 10, fd);
    size_t lastIndex = strlen(buf) - 1;
    buf[lastIndex] = '\0';

  	if(res==NULL){
  		break;
    }
  	fprintf(fc, "%s\n", buf);		    // scrivo su file .log
   	writeSocket(socketFd, buf);		// scrivo su socket fwc <--> ecu

    sleep(5);           // look: dovrà essere 10 secondi
    i++;
  }
}
