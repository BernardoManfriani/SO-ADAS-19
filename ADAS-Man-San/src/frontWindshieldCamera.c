#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include<signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "../lib/socketManager.h"
#include "../lib/fileManager.h"

#define SIGPARK SIGUSR1

int socketFd;
long int lastLineRead;

pid_t pidEcu;

FILE *readFd;
FILE *logFd;

void init();

void readFile(FILE *fd,FILE *fc);
void sigTermHandler();
void getLastLineRead();
char *readLine(FILE *fp);


int main() {
  pidEcu = getppid();

  signal(SIGTERM, sigTermHandler);     // dopo uno warning, salvo su utility.data il valore dell'ultima riga letta
  init();     // connesione ECU-SERVER + apertura files

  getLastLineRead();  // lastLineRead = valore dell'ultima riga letta

  readFile(readFd, logFd);            // leggo file -> 1.Scrivo su fwcSocket 2.Scrivo su camera.log

  return 0;
}

void init() {
  socketFd = connectClient("fwcSocket");

  openFile("../data/frontCamera.data","r", &readFd);
  openFile("../log/camera.log","w", &logFd);
}

void readFile(FILE *fd,FILE *fc){
  char buf[10];
  char *res;
  while(1) {
  	res=fgets(buf, 10, fd);
    size_t lastIndex = strlen(buf) - 1;
    buf[lastIndex] = '\0';

  	if(res==NULL){
  		break;
    }
  	fprintf(fc, "%s\n", buf);		    // scrivo su file camera.log
   	writeSocket(socketFd, buf);		// scrivo su socket fwcSocket

    sleep(10);
  }
}

void sigTermHandler(){
  FILE *fileUtility;
  openFile("../data/utility.data", "w", &fileUtility);
  lastLineRead = ftell(readFd);
  fprintf(fileUtility, "%ld\n", lastLineRead);
  fclose(fileUtility);
  fclose(readFd);

  exit(0);
}


void getLastLineRead() {
  FILE *fileUtility;
  openFile("../data/utility.data", "r", &fileUtility);
  char *lineNumber;

  lineNumber = readLine(fileUtility);
  lastLineRead = atol(lineNumber);

  fclose(fileUtility);
  fseek(readFd, lastLineRead, SEEK_SET);
}

char* readLine(FILE *fp){   // Long signed integer Capable of containing at least [−2,147,483,647, +2,147,483,647]
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