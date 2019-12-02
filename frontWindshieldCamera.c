#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "socketManager.h"
#include "fileManager.h"

#define NAME "socket"

int currentSpeed;
int socketFd;
FILE *readFd;
FILE *logFd;

void closeClient(int s);

void readFile(FILE *fd,FILE *fc);

int main() {
  currentSpeed = 0;
  printf("SENSORE fwc: attivo\n");

  socketFd = connectClient("socket");
  printf("SENSORE fwc: connection open\n");

  openFile("frontCamera.data","r", &readFd);      // look: PROSSIMO PASSO - APRIRE FILE .data 1.LEGGERE ROBA E SCRIVERE SU LOG  
  openFile("frontCamera.log","w", &logFd);

  readFile(readFd, logFd);            // leggo file -> 1.Scrivo su socket 2.Scrivo su .log

  closeClient(socketFd);
  printf("%s\n", "SENSORE fwc: connection close");

  return 0;
}

void closeClient(int socketFd) {
    close (socketFd); /* Close the socket */
}

void readFile(FILE *fd,FILE *fc){
  char buf[10];               // look: 10 va è sufficiente?
  char *res;
  int i = 0;
  while(i < 1) {             // look: PER ORA LEGGO SOLO 5 RIGHE DAL FILE
  	res=fgets(buf, 10, fd);

  	if(res==NULL)
  		break;

  	fprintf(fc, "%s", buf);		    // scrivo su file .log
   	writeSocket(socketFd, buf);		// scrivo su socket fwc <--> ecu

    sleep(1);           // look: dovrà essere 10 secondi
    i++;
  }
}