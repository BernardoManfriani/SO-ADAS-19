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

#include "socketManager.h"
#include "fileManager.h"

#define DEFAULT_PROTOCOL 0
#define READ 0
#define WRITE 1

pid_t pidBs;

FILE * logS;  //Camera Descriptor

//char logAction[30];

void createServer();
void action(char *a);
void sigTermHandler();

int main(int argc, char *argv[]) {

  printf("ATTUATORE sbw: attivo\n");

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
      //printf(".\n");
      sleep(1);
      fprintf(logS, "STO GIRANDO A %s\n", data);
    }

    kill(pidBs, SIGSTOP);
  } else {
    sleep(1);
    fprintf(logS, "NO ACTION\n");
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
  strcpy (serverUNIXAddress.sun_path, "sbwSocket"); /* Set name */    //ELIMINARE: Questo sbwSocket cos'è? Va levato?
  unlink ("sbwSocket"); /* Remove file if it already exists */        //ELIMINARE: Questo sbwSocket cos'è? Va levato?
  bind (serverFd, serverSockAddrPtr, serverLen);/*Create file*/
  listen (serverFd, 1); /* Maximum pending connection length */

  printf("ATTUATORE-SERVER sbw: wait client\n");
  clientFd = accept (serverFd, clientSockAddrPtr, &clientLen);	// bloccante
  printf("ATTUATORE-SERVER sbw: accept client\n");

  //fcntl(clientFd,F_SETFL,O_NONBLOCK); //Rende la read non bloccante    //ELIMINARE: Da rimettere non bloccante
  openFile("steer.log","w", &logS);
  char data[10];

  while (1) {/* Loop forever */ /* Accept a client connection */
    //printf("ATTUATORE-SERVER sbw: wait to read something\n");
    if(readSocket(clientFd, data)){
      //printf("ATTUATORE-SERVER sbw: leggo = '%s'\n", data);
      action(data);
    } else {
      //printf("ATTUATORE-SERVER sbw: end to read socket\n");

      fclose(logS);
      close (clientFd); /* Close the socket */
      exit (0);

    }

  }
}


void sigTermHandler() {
  signal(SIGTERM,SIG_DFL);
  kill(pidBs,SIGTERM);
  exit(0);
}
