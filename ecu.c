#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

#include<signal.h>

#include <sys/wait.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h> /* For AFUNIX sockets */

#include "socketManager.h"

#define NAME "socket"

#define DEFAULT_PROTOCOL 0
//#define NUMERO_SENSORI 5
//#define NUMERO_ATTUATORI 3
#define MAX_DATA_SIZE 100
#define READ 0
#define WRITE 1

int currentSpeed;

// PID ATTUATORI
pid_t pidTc;

// PID SENSORI - look: non considero pidSvc, pidPa perchè processi attivati al bisogno e non all'avvio del sistema (?)
pid_t pidFwc, pidFfr, pidBs;

// SOCKET FD
int tcSocketFd;


void startEcuSigHandler(int );
void start();

void creaSensori();				// look: fare TEMPLATE metodo creazione sensori
void fwcS(pid_t pidFwc);
void ffrS(pid_t pidFfr);
void bsS(pid_t pidBs);
void createServer();

void creaAttuatori();
int connectClient(char *socketName);

void writeShit (int socketFd);

int main() {
	//currentSpeed = 0;

	signal(SIGUSR1, startEcuSigHandler);
	pause();

	start();

	return 0;
}

void start() {
	creaSensori();                  // create ECU-SERVER
	creaAttuatori();				// create ECU-CLIENT
}

void startEcuSigHandler(int x) {
	//signal(SIGUSR1, startEcuSigHandler);      		// look: reset handler ?!
	printf("%s\n", "signal arrivata -> ECU STARTED");
}

void creaSensori() {
    
        pidFwc = fork();
        fwcS(pidFwc);

        pidBs = fork();
        bsS(pidBs);

        pidFfr = fork();
        ffrS(pidFfr);

		printf("%s", "ECU: create server\n");
		createServer();				// ECU-SERVER
		// look: preoccuparsi che il server aspetti i figli (ora abbiamo che dopo tot letture su socket usciamo)
}

void fwcS(pid_t pidFwc){				//front wind shield camera Sensore
    char *argv[] = {"./fwc", NULL}; 

    if(pidFwc < 0){
        perror("fork");
        exit(1);
    }
    if(pidFwc == 0) {
        execv(argv[0], argv);
        exit(0);
       
    }
}                                 

void ffrS(pid_t pidFfr){              	// forward facing radar Sensore
    char *argv[] = {"./ffr", NULL}; 

    if(pidFfr < 0){
        perror("fork");
        exit(1);
    }
    if (pidFfr==0){
        execv(argv[0], argv);
        exit(0);
    }
}


void bsS(pid_t pidBs){					// blind spot camera Sensore
    char *argv[] = {"./bs", NULL}; 

    if(pidBs < 0){
        perror("fork");
        exit(1);
    }
    if (pidBs==0)
    {
        execv(argv[0], argv);
        exit(0);
    }
}

void creaAttuatori() {
	char *argv[] = {"./fwc", NULL};

	pidTc = fork();
	if(pidTc < 0) {
		perror("fork");
		exit(0);
	}
	if(pidTc == 0) {				// ATTUATORI: throttle control
		execv(argv[0], argv);
		exit(0);
	} else {
		printf("%s", "ECU: CLIENT-connect to tcSocket\n");				// look: COSE DA FARE, CAPIRE IL GIRO DELL'ECU CLIENT
		tcSocketFd = connectClient("tcSocket");     // ECU-CLIENT
		sleep(5);
		
		writeShit(tcSocketFd);			// look: SCRIVERE DATI RICEVUTI DA SENSORI SU SOCKET ATTUATORI
		//wait(NULL);
		//exit(0);
	}
}

void createServer() {
	int serverFd, clientFd, serverLen, clientLen;
	struct sockaddr_un serverUNIXAddress; /*Server address */
	struct sockaddr_un clientUNIXAddress; /*Client address */
	struct sockaddr* serverSockAddrPtr; /*Ptr to server address*/
	struct sockaddr* clientSockAddrPtr;/*Ptr to client address*/

	/* Ignore death-of-child signals to prevent zombies */
	//signal (SIGCHLD, SIG_IGN);								// look: cosa fa realmente sta robba?!

	serverSockAddrPtr = (struct sockaddr*) &serverUNIXAddress;
	serverLen = sizeof (serverUNIXAddress);
	clientSockAddrPtr = (struct sockaddr*) &clientUNIXAddress;
	clientLen = sizeof (clientUNIXAddress);
	serverFd = socket (AF_UNIX, SOCK_STREAM, DEFAULT_PROTOCOL);

	serverUNIXAddress.sun_family = AF_UNIX; /* Set domain type */
	strcpy (serverUNIXAddress.sun_path, "socket"); /* Set name */
	unlink ("socket"); /* Remove file if it already exists */
	bind (serverFd, serverSockAddrPtr, serverLen);/*Create file*/
	listen (serverFd, 5); /* Maximum pending connection length - 5 sensori in ascolto */		// look: creo 1 server con 5 client VS creo 5 server con 1 client

	while (1) {/* Loop forever */ /* Accept a client connection */
		printf("ECU: SERVER-wait client\n");

		clientFd = accept (serverFd, clientSockAddrPtr, &clientLen);	// bloccante
		printf("ECU: SERVER-accept client\n");

		char data[30];
		if(fork () == 0) { /* Create child read ECU client */			// look: FARE fork() fuori dal while, CON FIGLIO CHE CREA ECU-SERVER E STA IN ASCOLTO
			printf("ECU: SERVER-wait to read something\n");
			while(readSocket(clientFd, data)) {							// look: cosa ritorna read se socket vuoto? (so che ritorna 0 se END FILE)
				printf("%s\n", data);
			}

			printf("ECU: SERVER-end to read socket\n");

			close (clientFd); /* Close the socket */
			exit (0); /* Terminate */
		} else {
			close (clientFd); /* Close the client descriptor */
			wait(NULL);			// WAIT CHILD READER - cosi facendo HMI verrà chiusa una volta letto qualcosa
			exit(0);	// look: o break
		}
	}
}

int connectClient(char *socketName){			// look: stesso metodo presente nei sensori(?)
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