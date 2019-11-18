#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include<signal.h>

#include <sys/wait.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h> /* For AFUNIX sockets */

#define NAME "socket"

#define DEFAULT_PROTOCOL 0
#define MAX_SOCKET_NAME_SIZE 5
#define SERVER_SOCKET_NUMBER 5
#define CLIENT_SOCKET_NUMBER 3
#define MAX_DATA_SIZE 100
#define READ 0
#define WRITE 1

// char sockets_name[MAX_SOCKET_NAME_SIZE]={
// "ffrSock",
// };

int currentSpeed;


pid_t pidTc;
pid_t pidFcw;

int tcSocketFd;


void createServer();
int connectClient(char* socketName);
int readFromSocket (int );
int readLines (int x, char *y);
void creaAttuatori();
void creaSensori();

void writeShit (int socketFd);

void startEcuSigHandler(int );
void start();

int main() {
	currentSpeed = 0;

	signal(SIGUSR1, startEcuSigHandler);
	pause();

	start();

	return 0;
}

void start() {
	creaSensori();                  // crea ECU-SERVER
	//creaAttuatori();
}

void startEcuSigHandler(int x) {
	//signal(SIGUSR1, startEcuSigHandler);      // reset handler ?!
	printf("%s\n", "signal arrivata -> ECU STARTED");
}

void creaSensori() {
	char *argv[] = {"./fwc", NULL}; 
	pidFcw = fork();
	if(pidFcw < 0) {
		perror("fork");
		exit(1);
	}
	if(pidFcw == 0) {               // fwc child process
		execv(argv[0], argv);
		exit(0);
		printf("1\n");
	} else {
		printf("%s", "creoserver\n");
		createServer();
		wait(NULL);
		printf("2\n");
	}
}

void creaAttuatori() {
	char *argv[6]; 
	pidTc = fork();
	if(pidTc < 0) {
		perror("fork");
		exit(0);
	}
	if(pidTc == 0) {  // throttle control child process
		argv[0] = "./tc";
		execv(argv[0], argv);
	} else {
		printf("%s", "creoserver\n");
		tcSocketFd = connectClient("tcSocket");     // create ecu client
		sleep(5);
		
		writeShit(tcSocketFd);
		exit(0);
	}
}

void createServer() {
	int serverFd, clientFd, serverLen, clientLen;
	struct sockaddr_un serverUNIXAddress; /*Server address */
	struct sockaddr_un clientUNIXAddress; /*Client address */
	struct sockaddr* serverSockAddrPtr; /*Ptr to server address*/
	struct sockaddr* clientSockAddrPtr;/*Ptr to client address*/

	/* Ignore death-of-child signals to prevent zombies */
	//signal (SIGCHLD, SIG_IGN);

	serverSockAddrPtr = (struct sockaddr*) &serverUNIXAddress;
	serverLen = sizeof (serverUNIXAddress);
	clientSockAddrPtr = (struct sockaddr*) &clientUNIXAddress;
	clientLen = sizeof (clientUNIXAddress);
	serverFd = socket (AF_UNIX, SOCK_STREAM, DEFAULT_PROTOCOL);

	serverUNIXAddress.sun_family = AF_UNIX; /* Set domain type */
	strcpy (serverUNIXAddress.sun_path, "socket"); /* Set name */
	unlink ("socket"); /* Remove file if it already exists */
	bind (serverFd, serverSockAddrPtr, serverLen);/*Create file*/
	listen (serverFd, 5); /* Maximum pending connection length - 5 sensori in ascolto */

	while (1) {/* Loop forever */ /* Accept a client connection */
		printf("SERVER: wait client\n");
		clientFd = accept (serverFd, clientSockAddrPtr, &clientLen);
		printf("SERVER: accept client\n");

		/*if (fork () == 0) { // Create child read ecu client
		    //while(1) {
		        //readFromSocket (clientFd); // Send data
		        //sleep(1);
		    //}
		    close (clientFd); // Close the socket
		    exit (0); // Terminate
		} else {
		    close (clientFd); // Close the client descriptor
		    exit (0);
		}*/
		char data[30];

		if(fork () == 0) { /* Create child read ECU client */
			printf("SERVER: wait to read something\n");
			while(readLines(clientFd, data)) {
				printf("%s", data);
			}

			close (clientFd); /* Close the socket */
			exit (0); /* Terminate */
		} else {
			close (clientFd); /* Close the client descriptor */
			wait(NULL);			// WAIT CHILD READER - cosi facendo HMI verrà chiusa una volta letto qualcosa
		}

		break;			// leggo una sola volta
	}
}

int readFromSocket(int fd) {
	char str[100];
	while (readLines (fd, str)); /* Read lines until end-of-input */
	printf ("%s\n", str);
}

int readLines(int fd, char *str) {
	int n;
	do { /* Read characters until ’\0’ or end-of-input */
		n = read(fd, str, 1); /* Read one character */
	} while (n > 0 && *str++ != '\0');
	return (n > 0);
}

int connectClient(char *socketName){
	int socketFd, serverLen;
	struct sockaddr_un serverUNIXAddress;
	struct sockaddr* serverSockAddrPtr;

	serverSockAddrPtr = (struct sockaddr*) &serverUNIXAddress;
	serverLen = sizeof (serverUNIXAddress);
	socketFd = socket (AF_UNIX, SOCK_STREAM, DEFAULT_PROTOCOL);
	serverUNIXAddress.sun_family = AF_UNIX; /* Server domain */
	strcpy (serverUNIXAddress.sun_path, socketName);/*Server name*/
	// int result = connect(socketFd, serverSockAddrPtr, serverLen);
	// if(result < 0){
	// 	return result;
	// }
	 int result;
	 do { /* Loop until a connection is made with the server */
		result = connect(socketFd, serverSockAddrPtr, serverLen);
		if (result == -1) sleep (1); /* Wait and then try again */
	} while (result == -1);

	printf("Ecu connessa con %s\n", socketName);
	return socketFd;
}

void writeShit (int socketFd) {
	char *s = "SHIT BRODER";
	write(socketFd, s, strlen (s) + 1);

	printf("SCRIVO SU SOCKET\n");
}