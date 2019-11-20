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
#define DEFAULT_PROTOCOL 0

int currentSpeed;
int socketFd;
FILE *readFd;
FILE *logFd;

int createClient();
void closeClient(int s);
int connectClient(char *socketName);

void readFile(FILE *fd,FILE *fc);

int main() {
  currentSpeed = 0;
  printf("SENSORE fwc: attivo\n");

  socketFd = connectClient("socket");
  printf("SENSORE fwc: CLIENT-connection open\n");

  openFile("frontCamera.data","r", &readFd);      // look: PROSSIMO PASSO - APRIRE FILE .data 1.LEGGERE ROBA E SCRIVERE SU LOG  
  openFile("frontCamera.log","w", &logFd);

  //printf("LEGGO read %d", readFd);
  //printf("LEGGO log %d", logFd);

  readFile(readFd, logFd);            // leggo file -> 1.Scrivo su socket 2.Scrivo su .log

  closeClient(socketFd);
  printf("%s\n", "SENSORE fwc: CLIENT-connection close");

  return 0;
}

int connectClient(char *socketName){
	int socketFd, serverLen;
	struct sockaddr_un serverUNIXAddress;
	struct sockaddr* serverSockAddrPtr;

	serverSockAddrPtr = (struct sockaddr*) &serverUNIXAddress;
	serverLen = sizeof (serverUNIXAddress);
	socketFd = socket (AF_UNIX, SOCK_STREAM, DEFAULT_PROTOCOL);
	serverUNIXAddress.sun_family = AF_UNIX;  /* Server domain */
	strcpy (serverUNIXAddress.sun_path, socketName);/*Server name*/
	int result = connect(socketFd, serverSockAddrPtr, serverLen);
 	if(result < 0){
 		return result;
 	}

 	return socketFd;
}

void closeClient(int socketFd) {
    close (socketFd); /* Close the socket */
}

void readFile(FILE *fd,FILE *fc){
  char buf[10];
  char *res;
  int i = 0;
  while(i < 5) {             // look: PER ORA LEGGO SOLO 5 RIGHE DAL FILE
  	res=fgets(buf, 10, fd);

  	if(res==NULL)
  		break;

  	fprintf(fc, "%s", buf);		    // scrivo su file .log
   	writeSocket(socketFd, buf);		// scrivo su socket fwc <--> ecu

    sleep(1);           // look: dovrà essere 10 secondi
    i++;
  }
}