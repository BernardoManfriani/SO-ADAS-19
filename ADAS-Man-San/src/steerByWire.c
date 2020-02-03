#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>

#include "../lib/socketManager.h"
#include "../lib/fileManager.h"

#define DEFAULT_PROTOCOL 0
#define READ 0
#define WRITE 1

pid_t pidBs;
pid_t pidWriter;

int status;
int pipeFd[2];

FILE * fileLog;

void initPipe();
void createServer();

void writeLog();
void sigTermHandler();


int main(int argc, char *argv[]) {
  initPipe();

  fcntl(pipeFd[READ], F_SETFL, O_NONBLOCK);   // rende la read su pipe non bloccante

  pidBs = fork();
  if(pidBs == 0) {
      argv[0] = "./bs"; 
      execv(argv[0],argv);

  } else {

    pidWriter = fork();
    if(pidWriter == 0) {      // child process writer on brake.log file
      close(pipeFd[WRITE]);
      writeLog();
      close(pipeFd[READ]);

    } else {        // father process listener on socket
      signal(SIGTERM, sigTermHandler);

      close(pipeFd[READ]);
      createServer();
    }
  }
}

void createServer() {
  int serverFd, clientFd, serverLen, clientLen;
  struct sockaddr_un serverUNIXAddress; /*Server address */
  struct sockaddr_un clientUNIXAddress; /*Client address */
  struct sockaddr* serverSockAddrPtr; /*Ptr to server address*/
  struct sockaddr* clientSockAddrPtr;/*Ptr to client address*/

  serverSockAddrPtr = (struct sockaddr*) &serverUNIXAddress;
  serverLen = sizeof (serverUNIXAddress);
  clientSockAddrPtr = (struct sockaddr*) &clientUNIXAddress;
  clientLen = sizeof (clientUNIXAddress);
  serverFd = socket (AF_UNIX, SOCK_STREAM, DEFAULT_PROTOCOL);

  serverUNIXAddress.sun_family = AF_UNIX; /* Set domain type */
  strcpy (serverUNIXAddress.sun_path, "sbwSocket"); /* Set name */
  unlink ("sbwSocket"); /* Remove file if it already exists */
  bind (serverFd, serverSockAddrPtr, serverLen);/*Create file*/
  listen (serverFd, 1); /* Maximum pending connection length */

  clientFd = accept (serverFd, clientSockAddrPtr, &clientLen);
  printf("ATTUATORE steer-by-wire: connected\n");

  char data[30];
  while(1) {
    if(readSocket(clientFd, data)){
      write(pipeFd[WRITE], data, strlen(data)+1);
    }
  }
}

void writeLog() {
  openFile("../log/steer.log", "w", &fileLog);
  sleep(1);
  char socketData[30];
  while(1) {
    if(read(pipeFd[READ], socketData, 30) > 0){
      kill(pidBs, SIGCONT);

      for (int i = 0; i < 4; i++) {
        fprintf(fileLog, "STO GIRANDO A %s\n", socketData);
        fflush(fileLog);

        sleep(1);
      }

      kill(pidBs, SIGSTOP);

    } else {
      fprintf(fileLog, "%s", "NO ACTION\n");
      fflush(fileLog);

      sleep(1);
    }
  }
}

void initPipe() {
  status = pipe(pipeFd);
  if(status != 0) {
    printf("Pipe error\n");
    exit(1);
  }
}

void sigTermHandler() {
  kill(pidWriter, SIGTERM);
  fclose(fileLog);
  kill(pidBs,SIGTERM);
  exit(0);
}
