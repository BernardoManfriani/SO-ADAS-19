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
pid_t pidTc, pidBbw, pidSbw;

// PID ECU-CLIENTS	-	clienti del socket aperto con gli attuatori
pid_t pidEcuClientTc, pidEcuClientBbw, pidEcuClientSbw;

// PID SENSORI - look: non considero pidSvc, pidPa perchè processi attivati al bisogno e non all'avvio del sistema (?)
pid_t pidFwc, pidFfr, pidBs;

// SOCKET FD
int tcSocketFd, bbwSocketFd, sbwSocketFd;

// Pipe - da ECU_SERVER a ECU-CLIENT
int pipe_server_to_ecu_client_tc[2];


void startEcuSigHandler(int );
void init();
void start();

void creaComponente(pid_t pidSens, char *argv[]);

void creaSensori();
void creaServer();

void creaEcuClients();
void creaEcuClientTc(pid_t );

void creaAttuatori();

void writeShit (int socketFd);

void processFwcData(char *data);

int main() {
	//currentSpeed = 0;

	signal(SIGUSR1, startEcuSigHandler);

	init();			// look: si mette qui per guadagnare un po di tempo

	pause();

	start();

	return 0;
}

void init() {		// inizializza le pipe------- look: SBAGLIATO facendo cosi tutti i processi hanno tutte le pipe aperte
					// Pero se non lo metto in start ECU SERVER e ECU CLIENT non hanno la struttura dati in comune
					// Quindi se si inizializzano le pipe qui, bisogna preoccuparci nei vari processi di chiudere le pipe che non si usano 

	pipe(pipe_server_to_ecu_client_tc);

}

void start() {
	creaSensori();
	creaAttuatori();

	creaEcuClients();		// ECU-CLIENTS

	creaServer();			// ECU-SERVER
}

void startEcuSigHandler(int x) {
	//signal(SIGUSR1, startEcuSigHandler);      		// look: reset handler ?!
	printf("%s\n", "signal arrivata -> ECU STARTED");
}

void creaSensori() {
    char *argv[2];

    argv[0] = "./fwc";
    pidFwc = fork();
	creaComponente(pidFwc, argv);			// Sensore front windshield camera

    argv[0] = "./bs";
    pidBs = fork();
	creaComponente(pidBs, argv);			// Sensore blind spot camera

    argv[0] = "./ffr";
    pidFfr = fork();
	creaComponente(pidFfr, argv);			// Sensore forward facing radar
}

void creaComponente(pid_t pidComp, char *argv[]){
    if(pidComp < 0){
        perror("fork");
        exit(1);
    }
    if(pidComp == 0) {
        execv(argv[0], argv);
        exit(0);
    }
}
				/* pagina 3 testo progetto: ECU-CLIENT chiede di connettersi con i 3 ATTUATORI-SERVER. I dati che gli manda sulla socket
					per mandare avanti la macchina, provengono da front windshied camera.*/
void creaEcuClients() {
	pidEcuClientTc = fork();
	creaEcuClientTc(pidEcuClientTc);

	/*pidEcuClientBbw = fork();
	creaEcuClient(pidEcuClientBbw);

	pidEcuClientSbw = fork();
	creaEcuClient(pidEcuClientSbw);*/

	//tcSocketFd = connectClient("tcSocket");
	//bbwSocketFd = connectClient("bbwSocket");	
	//sbwSocketFd = connectClient("sbwSocket");	
}


void creaEcuClientTc(pid_t pidEcuClientTc){		// look: per ora creo solo ECU-CLIENT - tc (a quanto pare è l'unico ?!?) non so a cosa puo servire
    if(pidEcuClientTc < 0){
        perror("fork");
        exit(1);
    }
    if(pidEcuClientTc == 0) {
		tcSocketFd = connectClient("tcSocket");
		printf("%s", "ECU-CLIENT: connected to tcSocket\n");

		sbwSocketFd = connectClient("sbwSocket");

    	close(pipe_server_to_ecu_client_tc[WRITE]);


    	char data[MAX_DATA_SIZE];
		//while(1){
			read(pipe_server_to_ecu_client_tc[READ], data,  MAX_DATA_SIZE);
			
			//printf("ECU-CLIENT: leggo = '%s'\n", data);
			processFwcData(data);

		//}
		//writeShit(tcSocketFd);		

		close(pipe_server_to_ecu_client_tc[READ]);		// look: ha sneso metterlo? non verrà mai eseguito
        exit(0);
    }
}



void creaAttuatori() {
    char *argv[2];

    argv[0] = "./tc";
    pidTc = fork();
	creaComponente(pidTc, argv);			// Attuatore throttle control

    argv[0] = "./bbw";
    pidBbw = fork();
	creaComponente(pidBbw, argv);			// Attuatore brake by wire

    argv[0] = "./sbw";
    pidSbw = fork();
	creaComponente(pidSbw, argv);			// Attuatore steer by wire
}

void creaServer() {					// look: per ora fa solo ECU SERVER - SENSORE fwc 
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
		printf("ECU-SERVER: wait client\n");

		clientFd = accept (serverFd, clientSockAddrPtr, &clientLen);	// bloccante
		printf("ECU-SERVER: accept client\n");

		char data[30];

		printf("ECU-SERVER: wait to read something\n");			// look: FARE fork() fuori dal while, CON FIGLIO CHE CREA ECU-SERVER E STA IN ASCOLTO ? 
		while(readSocket(clientFd, data)) {							// look: cosa ritorna read se socket vuoto? (so che ritorna 0 se END FILE)
			close(pipe_server_to_ecu_client_tc[READ]);

			write(pipe_server_to_ecu_client_tc[WRITE], data, strlen(data)+1);

			//printf("%s\n", data);
			close(pipe_server_to_ecu_client_tc[WRITE]);
		}

		printf("ECU-SERVER: end to read socket\n");

		close (clientFd); /* Close the socket */
		exit (0); /* Terminate */
	}
}

void writeShit (int socketFd) {
	char *s = "SHIT BRODER";
	write(socketFd, s, strlen (s) + 1);

	printf("SCRIVO SU SOCKET\n");
}

void processFwcData(char *data) {
	char *command;

	if(strcmp(data, "SINISTRA") == 0 || strcmp(data, "DESTRA") == 0) {
		//command = malloc(strlen(data) + 1);		// copia data e lo incollo in command
		//sprintf(command, "%s", data);
		writeSocket(sbwSocketFd, data);				// scrivo a steer-by-wire
	} else if(strcmp(data, "PERICOLO") == 0) {
		
	} else {

	}

}