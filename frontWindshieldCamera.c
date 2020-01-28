#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include<signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "socketManager.h"
#include "fileManager.h"

int socketFd;
long int lastLineRead;
FILE *readFd;
FILE *logFd;

void closeClient(int s);

void readFile(FILE *fd,FILE *fc);
void writeLastLineRead();
void getLastLineRead();
char* readLine(FILE *fp);

int main() {
  signal(SIGTERM, writeLastLineRead);
  printf("SENSORE fwc: attivo\n");

  socketFd = connectClient("fwcSocket");
  printf("SENSORE fwc: connection open\n");

  openFile("frontCamera.data","r", &readFd);
  openFile("camera.log","w", &logFd);

  getLastLineRead();  // setto lastLineRead con il valore dell'ultima riga letta da file frontCamera.data (valore scritto in utility.data)

  //printf("------------------------ ULTIMA RIGA LETTA: '%li' --------------------\n", lastLineRead);

  readFile(readFd, logFd);            // leggo file -> 1.Scrivo su fwcSocket 2.Scrivo su .log

  closeClient(socketFd);
  printf("%s\n", "SENSORE fwc: connection close");

  return 0;
}

void closeClient(int socketFd) {
    close(socketFd); /* Close the socket */
}

void readFile(FILE *fd,FILE *fc){
  char buf[20];               // look: 20 va  bene? è sufficiente?
  char *res;
  int i = 0;
  while(i < 12) {
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

void writeLastLineRead(){
  FILE *fileUtility;
  openFile("utility.data", "w", &fileUtility);
  lastLineRead = ftell(readFd);
  fprintf(fileUtility, "%ld\n", lastLineRead);
  fclose(fileUtility);
  fclose(readFd);
  exit(0);
}


void getLastLineRead() {
  FILE *fileUtility;
  openFile("utility.data", "r", &fileUtility);
  char *lineNumber;

  lineNumber = readLine(fileUtility);
  lastLineRead = atol(lineNumber);

  fclose(fileUtility);
  fseek(readFd, lastLineRead, SEEK_SET);
}

char* readLine(FILE *fp){   // Long signed integer Capable of containing at least [−2,147,483,647, +2,147,483,647] => 10 cifre o 8 ?!?!
  char *lineBuffer = (char *)malloc(sizeof(char) * 10);
  int count = 0;

  char ch = getc(fp);
  while ((ch != '\n') && (ch != EOF)) {
    lineBuffer[count] = ch;
    count++;  
    ch = getc(fp);
  }
    return lineBuffer;
}