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

FILE * fileLog;

void createServer();
void action(char *a);
void sigTermHandler();


int main(int argc, char *argv[]) {
  pidBs = fork();
  if(pidBs == 0) {
      argv[0] = "./bs"; 
      execv(argv[0],argv);
  } else {
    signal(SIGTERM, sigTermHandler);
    createServer();
  }

  return 0;
}

void action(char *data){

  if (strcmp(data,"DESTRA") == 0 || strcmp(data, "SINISTRA") == 0){
    kill(pidBs, SIGCONT);

    for (int i = 0; i < 4; i++) {

      fprintf(fileLog, "STO GIRANDO A %s\n", data);
      fflush(fileLog);

      sleep(1);
    }

    kill(pidBs, SIGSTOP);
  } else {
    printf("STEER LEGGE ALTRO\n");
    fprintf(fileLog, "NO ACTION\n");
    fflush(fileLog);
    
    sleep(1);
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
  printf("CLIENT-SERVER: sbwSocket connected\n");

  openFile("../log/steer.log","w", &fileLog);
  char data[10];

  while (1) {
    if(readSocket(clientFd, data)){
      action(data);
    }
  }
}

void sigTermHandler() {
  //signal(SIGTERM,SIG_DFL);
  fclose(fileLog);
  kill(pidBs,SIGTERM);
  exit(0);
}
