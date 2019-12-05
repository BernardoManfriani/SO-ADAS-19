#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>

#include<signal.h>

#include <sys/wait.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h> /* For AFUNIX sockets */

#include "socketManager.h"

#define SIGSTART SIGUSR1
#define SIGDANGER SIGUSR2

#define DEFAULT_PROTOCOL 0
#define NUMERO_SENSORI 1
//#define NUMERO_ATTUATORI 3
#define MAX_DATA_SIZE 20
#define READ 0
#define WRITE 1

int currentSpeed;

// PID ATTUATORI
pid_t pidTc, pidBbw, pidSbw;

// PID SENSORI - look: non considero pidSvc, pidPa perchè processi attivati al bisogno e non all'avvio del sistema (?)
pid_t pidFwc, pidFfr, pidBs;

// PID ECU-CLIENTS	-	clienti del socket aperto con gli attuatori
pid_t pidEcuClientFwcManager, pidEcuClientBbw, pidEcuClientSbw;

// PIPE - da ECU_SERVER a ECU-CLIENT
int pipe_fwc_data[2];

// PID ECU-SERVERS
pid_t pidEcuServers[NUMERO_SENSORI];

// Strutture dati server-client
int serverLen, clientLen;
struct sockaddr_un serverUNIXAddress; /*Server address */
struct sockaddr* serverSockAddrPtr; /*Ptr to server address*/
struct sockaddr_un clientUNIXAddress; /*Client address */
struct sockaddr* clientSockAddrPtr;/*Ptr to client address*/

// Nomi socket sensori
const char *nomeSocketSensori[NUMERO_SENSORI] = {
	"fwcSocket"
};

// FD SOCKET ATTUATORI
int tcSocketFd, bbwSocketFd, sbwSocketFd;

// FD SOCKET SENSORI
int fdSocketSensori[NUMERO_SENSORI];

void startEcuSigHandler(int );
void init();
void start();

void creaComponente(pid_t pidSens, char *argv[]);

void creaSensori();
void creaServer();

void creaEcuClients();
void fwcDataManager(pid_t );

void creaAttuatori();

void processFwcData(char *data);

int main() {

	signal(SIGSTART, startEcuSigHandler);

	init();			// look: si mette qui per guadagnare un po di tempo

	pause();

	start();

	return 0;
}

void init() {		// inizializza le pipe------- look: SBAGLIATO facendo cosi tutti i processi hanno tutte le pipe aperte
					// Pero se non lo metto in start ECU SERVER e ECU CLIENT non hanno la struttura dati in comune
					// Quindi se si inizializzano le pipe qui, bisogna preoccuparci nei vari processi di chiudere le pipe che non si usano 

	currentSpeed = 0;

	pipe(pipe_fwc_data);

	serverSockAddrPtr = (struct sockaddr*) &serverUNIXAddress;
	serverLen = sizeof (serverUNIXAddress);
	clientSockAddrPtr = (struct sockaddr*) &clientUNIXAddress;
	clientLen = sizeof (clientUNIXAddress);
	serverUNIXAddress.sun_family = AF_UNIX; /* Set domain type */

}

void start() {
	creaSensori();
	//creaAttuatori();

	//creaEcuClients();		// ECU-CLIENTS

	creaServer();			// ECU-SERVER

	//talkToHmi(); //-------------------------RIMASTO QUA
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

    /*argv[0] = "./bs";
    pidBs = fork();
	creaComponente(pidBs, argv);			// Sensore blind spot camera

    argv[0] = "./ffr";
    pidFfr = fork();
	creaComponente(pidFfr, argv);			// Sensore forward facing radar*/
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
	pidEcuClientFwcManager = fork();
	fwcDataManager(pidEcuClientFwcManager);

	/*pidEcuClientBbw = fork();
	creaEcuClient(pidEcuClientBbw);

	pidEcuClientSbw = fork();
	creaEcuClient(pidEcuClientSbw);*/

	//tcSocketFd = connectClient("tcSocket");
	//bbwSocketFd = connectClient("bbwSocket");	
	//sbwSocketFd = connectClient("sbwSocket");	
}


void fwcDataManager(pid_t pidEcuClientFwcManager){		// look: nome non corretto -- lui fa da manager dei dati ricevuti da fw
    if(pidEcuClientFwcManager < 0){
        perror("fork");
        exit(1);
    }
    if(pidEcuClientFwcManager == 0) {
		/*tcSocketFd = connectClient("tcSocket");
		printf("%s", "ECU-CLIENT: connected to tcSocket\n");

		sbwSocketFd = connectClient("sbwSocket");
		printf("%s", "ECU-CLIENT: connected to sbwSocket\n");

		bbwSocketFd = connectClient("bbwSocket");
		printf("%s", "ECU-CLIENT: connected to bbwSocket\n");*/

    	close(pipe_fwc_data[WRITE]);


    	char data[MAX_DATA_SIZE];
		//while(1){
			read(pipe_fwc_data[READ], data, MAX_DATA_SIZE);
			//printf("ECU-CLIENT: leggo = '%s'\n", data);
			processFwcData(data);

		//}		

		close(pipe_fwc_data[READ]);		// look: ha sneso metterlo? non verrà mai eseguito
        exit(0);
    }
}

void creaServer() {					// look: per ora fa solo ECU SERVER - SENSORE fwc 
	int clientFd;

	int i = 0;
	for(i = 0; i < NUMERO_SENSORI; i++) {
		pidEcuServers[i] = fork();
		if(pidEcuServers[i] == 0) {
			fdSocketSensori[i] = socket (AF_UNIX, SOCK_STREAM, DEFAULT_PROTOCOL);	// file descriptor socket sensori

			strcpy (serverUNIXAddress.sun_path, nomeSocketSensori[i]); /* Set name */
			unlink (nomeSocketSensori[i]);		/* Remove file if it already exists */
			bind (fdSocketSensori[i], serverSockAddrPtr, serverLen);/*Create file*/
			listen (fdSocketSensori[i], 1);	/* Maximum pending connection length */

			printf("ECU-SERVER: wait client\n");
			clientFd = accept (fdSocketSensori[i], clientSockAddrPtr, &clientLen);	// Accept a client connection - bloccante
			printf("ECU-SERVER: accept client\n");

			close(pipe_fwc_data[READ]);
			
			char data[MAX_DATA_SIZE];

			printf("ECU-SERVER: wait to read something\n"); 
			while (readSocket(clientFd, data)) {			// look: cosa ritorna read se socket vuoto? (so che ritorna 0 se END FILE)
				if(strcmp(nomeSocketSensori[i], "fwcSocket") == 0) {
					//write(pipe_fwc_data[WRITE], data, strlen(data)+1);
					printf("CAZZOZOZOZ\n");
				}
			}

			close(pipe_fwc_data[WRITE]);

			printf("ECU-SERVER: end to read socket\n");

			close (clientFd); /* Close the socket */
			exit (0); /* Terminate */
		} else {
			wait(NULL);
			printf("ARRIVO QUA ???\n");
			exit(0);
		}
	}
}

void processFwcData(char *data) {
	char command[MAX_DATA_SIZE];

	if(strcmp(data, "DESTRA") == 0 || strcmp(data, "SINISTRA") == 0) {
		//command = malloc(strlen(data) + 1);		// copia data e lo incollo in command
		//sprintf(command, "%s", data);

		writeSocket(sbwSocketFd, data);				// scrivo a steer-by-wire
	} else if(strcmp(data, "PERICOLO") == 0) {

		kill(pidBbw, SIGDANGER);// look: invio segnale di pericolo a break by wire
		
	} else {
		printf("HO LETTO UN NUMERO\n");

		int nextSpeed = atoi(data);
		int deltaSpeed;

		if(nextSpeed > currentSpeed) {
			deltaSpeed = nextSpeed - currentSpeed;
			sprintf(command, "INCREMENTO %d", deltaSpeed);	// push stringa "INCREMENTO X" in command

			currentSpeed = nextSpeed;

			//writeSocket(sbwSocketFd, data);				// scrivo a throttle-control
		} else if (nextSpeed < currentSpeed) {
			deltaSpeed = currentSpeed - nextSpeed;
			sprintf(command, "FRENO %d", deltaSpeed);	// push stringa "INCREMENTO X" in command

			currentSpeed = nextSpeed;

			//writeSocket(bbwSocketFd, data);				// scrivo a brake-by-wire
		}

		//write to talkToHmi();
	}

}