#include<stdio.h>
#include<stdlib.h>);
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



pid_t pid_sterzata;

FILE * logS;  //Camera Descriptor

//char logAction[30];

void createServer();

void action(pid_t pid_sterzata, char *a);


int main() {

  printf("ATTUATORE sbw: attivo\n");

  createServer();

  return 0;
}

void action(pid_t  pid_sterzata, char *data){

  if (strcmp(data,"DESTRA") == 0 || strcmp(data, "SINISTRA") == 0){
    for (int i = 0; i < 4; i++) {
      sleep(1);
      fprintf(logS, "STO GIRANDO A %s\n", data);
    }
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
  strcpy (serverUNIXAddress.sun_path, "sbwSocket"); /* Set name */
  unlink ("sbwSocket"); /* Remove file if it already exists */
  bind (serverFd, serverSockAddrPtr, serverLen);/*Create file*/
  listen (serverFd, 1); /* Maximum pending connection length */





  printf("ATTUATORE-SERVER sbw: wait client\n");
  clientFd = accept (serverFd, clientSockAddrPtr, &clientLen);	// bloccante
  printf("ATTUATORE-SERVER sbw: accept client\n");

  fcntl(clientFd,F_SETFL,O_NONBLOCK); //Rende la read non bloccante
  openFile("steer.log","w", &logS);
  char data[10];

  while (1) {/* Loop forever */ /* Accept a client connection */


    printf("ATTUATORE-SERVER sbw: wait to read something\n");

    if(readSocket(clientFd, data)){
      printf("ATTUATORE-SERVER sbw: leggo = '%s'\n", data);
      action(pid_sterzata, data);
    } else {
      printf("ATTUATORE-SERVER sbw: end to read socket\n");

      fclose(logS);
      close (clientFd); /* Close the socket */
      exit (0);

    }

  }

}
