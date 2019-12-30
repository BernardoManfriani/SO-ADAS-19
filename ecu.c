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
#define SIGPARK SIGUSR1
#define SIGDANGER SIGUSR2

#define DEFAULT_PROTOCOL 0
#define NUMERO_SENSORI 1
//#define NUMERO_ATTUATORI 3
#define MAX_DATA_SIZE 100
#define READ 0
#define WRITE 1

int currentSpeed;

pid_t pidHmi, pidEcu;

// PID ATTUATORI
pid_t pidTc, pidBbw, pidSbw;

// PID SENSORI - look: non considero pidSvc, pidPa perchè processi attivati al bisogno e non all'avvio del sistema (?)
pid_t pidFwc, pidFfr, pidBs;

// PID ECU-CLIENTS	-	clienti del socket aperto con gli attuatori
pid_t pidEcuClientFwcManager, pidEcuClientFfrManager;

// PIPE - da ECU_SERVER a ECU-CLIENT
int pipe_fwc_data[2], pipe_ffr_data[2];

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
	"fwcSocket",
	//"ffrSocket"
};

// FD SOCKET ATTUATORI
int tcSocketFd, bbwSocketFd, sbwSocketFd;

// FD SOCKET SENSORI
int fdSocketSensori[NUMERO_SENSORI];

char *startMode;

void startEcuSigHandler();
void parkingHandler();
void endParkingHandler();
void init();
void start();

void creaComponente(pid_t pidSens, char *argv[]);

void creaSensori();
void creaServers();

void creaEcuClients();
void fwcDataManager(pid_t );
void ffrDataManager(pid_t );

void creaAttuatori();

void processFwcData(char *data);
void decodeFfrData(unsigned char *data);

int main(int argc, char *argv[]) {
	startMode = argv[1];		// salvo modalità di avvio
	pidHmi = getppid();
	pidEcu = getpid();

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
	pipe(pipe_ffr_data);

	serverSockAddrPtr = (struct sockaddr*) &serverUNIXAddress;
	serverLen = sizeof (serverUNIXAddress);
	clientSockAddrPtr = (struct sockaddr*) &clientUNIXAddress;
	clientLen = sizeof (clientUNIXAddress);
	serverUNIXAddress.sun_family = AF_UNIX; /* Set domain type */

}

void start() {
	creaSensori();
	creaAttuatori();

	creaEcuClients();		// ECU-CLIENTS

	creaServers();			// ECU-SERVER

	//talkToHmi(); //----------------------------------------------------------------------------RIMASTO QUA
}

void startEcuSigHandler() {
	signal(SIGPARK, parkingHandler);			// signal gestione input PARCHEGGIO
	printf("%s\n", "signal arrivata -> ECU STARTED");
}

void parkingHandler() {
	signal(SIGPARK, parkingHandler);			// signal per ricevere segnale fino parcheggio da parte di brake by wire
	if(bbwSocketFd == 0) {						// look: cosa succede se è gia stata aperta una conn con bbw? =>bbwSocketFd variabile globale
		bbwSocketFd = connectClient("bbwSocket");
	}

	char *parkCommand;
	sprintf(parkCommand, "%s %d", "PARCHEGGIO", currentSpeed);
	writeSocket(bbwSocketFd, parkCommand);

	// look: GUARDA PRIMO FACOLTATIVO PAG 4 => DOVREI KILLARE GLI ALTRI ATTUATORI tc, sbw
}

void endParkingHandler() {
	//look: GUARDA PAGINA 3
	/*Quando la velocità raggiunge 0, Central ECU attiva Park assist ultrasonic sensors e Surround view cameras. 
	Se la Central ECU non riceve da nessuno dei due, per 30 secondi, uno dei valori: i) 0x172A, ii) 0xD693, iii) 0x, 
	iv) 0xBDD8, v) 0xFAEE, vi) 0x4300, l’auto è parcheggiata e la missione termina. 
	Altrimenti, la Central ECU ri-avvia la procedura di PARCHEGGIO, attivando attiva Park assist ultrasonic sensors e Surround view cameras.*/
}

void creaSensori() {
    char *argv[3];
    argv[1] = startMode;			// look: viene passato a tutti i sensori => noi lo utilizzeremo solo dove necessario
    argv[2] = NULL;			// look: ultimo elemento deve essere un null pointer


    argv[0] = "./fwc";
    pidFwc = fork();
	creaComponente(pidFwc, argv);			// Sensore front windshield camera*/

    /*argv[0] = "./ffr";
    pidFfr = fork();
	creaComponente(pidFfr, argv);			// Sensore forward facing radar

    /*argv[0] = "./bs";
    pidBs = fork();
	creaComponente(pidBs, argv);			// Sensore blind spot camera*/
}

void creaAttuatori() {
    char *argv[2];

    argv[0] = "./tc";
    pidTc = fork();
	creaComponente(pidTc, argv);			// Attuatore throttle control*/

    argv[0] = "./bbw";
    pidBbw = fork();
	creaComponente(pidBbw, argv);			// Attuatore brake by wire*/

   /* argv[0] = "./sbw2";					// look: non funziona steer by wire - non mi legge null => non stampa NO ACTION
    pidSbw = fork();
	creaComponente(pidSbw, argv);			// Attuatore steer by wire*/
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

	pidEcuClientFfrManager = fork();
	ffrDataManager(pidEcuClientFfrManager);
}

void fwcDataManager(pid_t pidEcuClientFwcManager){
    if(pidEcuClientFwcManager < 0){
        perror("fork");
        exit(1);
    }
    if(pidEcuClientFwcManager == 0) {
		tcSocketFd = connectClient("tcSocket");
		printf("%s", "ECU-CLIENT: connected to tcSocket\n");

		bbwSocketFd = connectClient("bbwSocket");
		printf("%s", "ECU-CLIENT: connected to bbwSocket\n");

		/*sbwSocketFd = connectClient("sbwSocket");
		printf("%s", "ECU-CLIENT: connected to sbwSocket\n");*/

    	close(pipe_fwc_data[WRITE]);

    	char data[MAX_DATA_SIZE];
		while(read(pipe_fwc_data[READ], data, MAX_DATA_SIZE)){
			processFwcData(data);
		}		

		printf("CHIUSO MANAGER FCW\n");
		close(pipe_fwc_data[READ]);		// look: ha sneso metterlo? non verrà mai eseguito
        exit(0);
    }
}

void ffrDataManager(pid_t pidEcuClientFfrManager){
    if(pidEcuClientFfrManager < 0){
        perror("fork");
        exit(1);
    }
    if(pidEcuClientFfrManager == 0) {
    	close(pipe_ffr_data[WRITE]);

    	// look: devo riconoscere se all'interno del dato ci sono quel tipo di sequenze da testo progetto -> in tal caso kill a brake-by-wire
    	char data[MAX_DATA_SIZE];
		while(read(pipe_ffr_data[READ], data, MAX_DATA_SIZE)){
			printf("FFR MANAGER: leggo\n");
			decodeFfrData(data);
		}		

		printf("CHIUSO MANAGER FFR\n");
		close(pipe_ffr_data[READ]);		// look: ha senso metterlo? non verrà mai eseguito
        exit(0);
    }
}

void creaServers() {					// look: per ora fa solo ECU SERVER - SENSORE fwc 
	int clientFd;
	close(pipe_fwc_data[READ]);
	close(pipe_ffr_data[READ]);

	for(int i = 0; i < NUMERO_SENSORI; i++) {
		pidEcuServers[i] = fork();
		if(pidEcuServers[i] == 0) {
			fdSocketSensori[i] = socket (AF_UNIX, SOCK_STREAM, DEFAULT_PROTOCOL);	// file descriptor socket sensori

			strcpy (serverUNIXAddress.sun_path, nomeSocketSensori[i]); /* Set name */
			unlink (nomeSocketSensori[i]);		/* Remove file if it already exists */
			bind (fdSocketSensori[i], serverSockAddrPtr, serverLen);/*Create file*/
			listen (fdSocketSensori[i], 1);	/* Maximum pending connection length */

			printf("ECU-SERVER %s: wait client\n", nomeSocketSensori[i]);
			clientFd = accept (fdSocketSensori[i], clientSockAddrPtr, &clientLen);	// Accept a client connection - bloccante
			printf("ECU-SERVER %s: accept client\n", nomeSocketSensori[i]);
			
			char data[MAX_DATA_SIZE];

			printf("ECU-SERVER %s: wait to read something\n", nomeSocketSensori[i]);
			while (readSocket(clientFd, data)) {			// look: cosa ritorna read se socket vuoto? (so che ritorna 0 se END FILE)
				if(strcmp(nomeSocketSensori[i], "fwcSocket") == 0) {
					write(pipe_fwc_data[WRITE], data, strlen(data)+1);
					//printf("LEGGO ROBA DA FWC SOCKET\n");
				} else if(strcmp(nomeSocketSensori[i], "ffrSocket") == 0) {
					//printf("LEGGO ROBA DA FFR SOCKET --------\n");
					write(pipe_ffr_data[WRITE], data, strlen(data)+1);
				}
			}

			printf("ECU-SERVER %s: end to read socket PID = %d\n", nomeSocketSensori[i], getpid());

			kill(pidEcuClientFwcManager, SIGKILL);		// look: togliere questa riga?!?!?

			close (clientFd); /* Close the socket */
			exit (0); /* Terminate */
		} /*else {
			wait(NULL);
			printf("processo padre di %s chiude\n", nomeSocketSensori[i]);
			exit(0);
		}*/
	}

	// look: creati i vari server esco e ECU termina - non va bene
	close(pipe_fwc_data[WRITE]);
	close(pipe_ffr_data[WRITE]);
	exit(0);		// look: questo exit() farà concludere la ECU e qundi a cascata hmi. Dobbiamo fare in modo che aspetti tutti
}

void processFwcData(char *data) {
	char command[MAX_DATA_SIZE];

	if(strcmp(data, "DESTRA") == 0 || strcmp(data, "SINISTRA") == 0) {
		//command = malloc(strlen(data) + 1);		// copia data e lo incollo in command
		//sprintf(command, "%s", data);

		printf("ECU-CLIENT: leggo DESTRA/SINISTRA\n");
		//writeSocket(sbwSocketFd, data);				// scrivo a steer-by-wire
	} else if(strcmp(data, "PERICOLO") == 0) {
		printf("ECU-CLIENT: leggo PERICOLO\n");

		currentSpeed = 0;
		kill(pidBbw, SIGDANGER);	// invio segnale di pericolo a break by wire
		kill(pidHmi, SIGDANGER);	// invio segnale di pericolo a HMI

	} else {				// viene letto un numero
		int nextSpeed = atoi(data);
		int deltaSpeed;

		if(nextSpeed > currentSpeed) {
			deltaSpeed = nextSpeed - currentSpeed;
			sprintf(command, "INCREMENTO %d", deltaSpeed);	// push stringa "INCREMENTO X" in command

			currentSpeed = nextSpeed;

			writeSocket(tcSocketFd, command);	// scrivo a throttle-control
		} else if (nextSpeed < currentSpeed) {
			deltaSpeed = currentSpeed - nextSpeed;
			sprintf(command, "FRENO %d", deltaSpeed);	// push stringa "INCREMENTO X" in command

			currentSpeed = nextSpeed;

			writeSocket(bbwSocketFd, command);	// scrivo a brake-by-wire
		}

		//write to talkToHmi();
	}
}

void decodeFfrData(unsigned char *data) {
    unsigned char errValues[] = {0xA0, 0x0F, 0xB0, 0x72, 0x2F, 0xA8, 0x83, 0x59, 0xCE, 0x23};

    for(int i = 0; i < 8; ++i){				// look: STAMPA VALORE LETTO
        printf("%02X ", data[i]);
    }
    printf("\n");

    for(int i = 0; i < 10; i++) {
		for(int j = 0; j < 8; j++) {
			if(data[j] == errValues[i]) {
				printf("FFR MANAGER: TROVATO HEX SIMILE - ERROR %02X\n", data[j]);
				kill(pidBbw, SIGDANGER); 	// invio segnale di pericolo a brake by wire

				// look: -----ATTENZIONE------- sembra che ad eseguire in conseguenza il progetto, ffr non apre i file per leggere roba

				kill(pidHmi, SIGDANGER);	// invio segnale di pericolo a HMI
				return ;
			}
		}
    }

	printf("BELLA\n");
}