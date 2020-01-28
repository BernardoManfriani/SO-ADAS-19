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
#include "fileManager.h"

#define SIGSTART SIGUSR1
#define SIGPARK SIGUSR1
#define SIGDANGER SIGUSR2
#define SIGRESETPARK SIGUSR2

#define DEFAULT_PROTOCOL 0
#define MAX_DATA_SIZE 100
#define READ 0
#define WRITE 1


char *startMode;
int currentSpeed;


pid_t pidHmi, pidEcu;

// PID ATTUATORI
pid_t pidTc, pidBbw, pidSbw;

// PID SENSORI - look: non considero pidSvc, pidPa perchè processi attivati al bisogno e non all'avvio del sistema (?)
pid_t pidFwc, pidFfr, pidPa;

// PID ECU-CLIENTS  - clienti del socket aperto con gli attuatori
pid_t pidEcuClientFwcManager, pidEcuClientFfrManager, pidEcuClientBsManager;

// PID ECU-SERVERS
pid_t pidFwcEcuServer, pidFfrEcuServer, pidPaEcuServer, pidBsEcuServer;


// PIPE - da ECU-SERVER a ECU-CLIENT
int pipe_fwc_data[2], pipe_ffr_data[2], pipe_bs_data[2], pipe_pa_data[2];

// PIPE per comunicazione con hmi
int pipe_ecu_logger[2];


// FD SOCKET ATTUATORI
int tcSocketFd, bbwSocketFd, sbwSocketFd;

// FD SOCKET SENSORI
int fwcSocketFd, ffrSocketFd, paSocketFd, bsSocketFd;

// Strutture dati server-client
int serverLen, clientLen;
struct sockaddr_un serverUNIXAddress; /*Server address */
struct sockaddr* serverSockAddrPtr; /*Ptr to server address*/
struct sockaddr_un clientUNIXAddress; /*Client address */
struct sockaddr* clientSockAddrPtr;/*Ptr to client address*/


void startEcuSigHandler();
void parkingHandler();
void endParkingHandler();
void init();
void start();

void creaComponente(pid_t pidSens, char *argv[]);

void creaSensori();

void creaServers();
void creaServer(pid_t pidEcuServer, int fdSocketSensore, char *nomeSocketSensore);

void creaParkAssist();

void creaDataManagers();
void fwcDataManager(pid_t );
void ffrDataManager(pid_t );
void bsDataManager(pid_t );

void creaAttuatori();

void processFwcData(char *data);
void decodeFfrData(unsigned char *data);
void decodeBsData(unsigned char *data);
void decodePaData(unsigned char *data);

void ecuLogger();
int getCurrentSpeed(char *);

void endProgram();

int readPipe (int pipeFd, char *data);

int main(int argc, char *argv[]) {
	startMode = argv[1];		// salvo modalità di avvio
	pidHmi = getppid();
	pidEcu = getpid();

	signal(SIGSTART, startEcuSigHandler);

	init();			// look: si mette qui per guadagnare un po di tempo

	pause();

	start();

	printf("ECU TERMINA\n");

	return 0;
}

void init() {		// inizializza le pipe------- look: SBAGLIATO facendo cosi tutti i processi hanno tutte le pipe aperte
					// Pero se non lo metto in start ECU SERVER e ECU CLIENT non hanno la struttura dati in comune
					// Quindi se si inizializzano le pipe qui, bisogna preoccuparci nei vari processi di chiudere le pipe che non si usano

	currentSpeed = 0;

	pipe(pipe_fwc_data);
	pipe(pipe_ffr_data);
	pipe(pipe_bs_data);
	pipe(pipe_pa_data);
	pipe(pipe_ecu_logger);

	serverSockAddrPtr = (struct sockaddr*) &serverUNIXAddress;
	serverLen = sizeof (serverUNIXAddress);
	clientSockAddrPtr = (struct sockaddr*) &clientUNIXAddress;
	clientLen = sizeof (clientUNIXAddress);
	serverUNIXAddress.sun_family = AF_UNIX; /* Set domain type */

}

void start() {
	creaSensori();
	creaAttuatori();

	creaDataManagers();		// ECU-CLIENTS

	creaServers();			// ECU-SERVER

	ecuLogger();
}

void startEcuSigHandler() {
	signal(SIGPARK, parkingHandler);			// signal gestione input PARCHEGGIO
	printf("%s\n", "signal arrivata -> ECU STARTED");
}

void creaSensori() {
	char *argv[3];
	argv[1] = startMode;			// look: viene passato a tutti i sensori => noi lo utilizzeremo solo dove necessario
	argv[2] = NULL;			// look: ultimo elemento deve essere un null pointer

	argv[0] = "./fwc";
	pidFwc = fork();
	creaComponente(pidFwc, argv);			// Sensore front windshield camera*/

	argv[0] = "./ffr";
	pidFfr = fork();
	creaComponente(pidFfr, argv);			// Sensore forward facing radar*/
}

void creaAttuatori() {
	char *argv[3];
	argv[1] = startMode;			// look: viene passato a tutti i sensori => noi lo utilizzeremo solo dove necessario
	argv[2] = NULL;			// look: ultimo elemento deve essere un null pointer

    argv[0] = "./tc";
    pidTc = fork();
	creaComponente(pidTc, argv);			// Attuatore throttle control*/

	argv[0] = "./bbw";
    pidBbw = fork();
	creaComponente(pidBbw, argv);			// Attuatore brake by wire*/

    argv[0] = "./sbw";					// look: non funziona steer by wire - non mi legge null => non stampa NO ACTION
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
void creaDataManagers() {
	pidEcuClientFwcManager = fork();
	fwcDataManager(pidEcuClientFwcManager);

	pidEcuClientFfrManager = fork();
	ffrDataManager(pidEcuClientFfrManager);

	pidEcuClientBsManager = fork();
	bsDataManager(pidEcuClientBsManager);
}

void fwcDataManager(pid_t pidEcuClientFwcManager){
    if(pidEcuClientFwcManager < 0){
        perror("fork");
        exit(1);
    }
    if(pidEcuClientFwcManager == 0) {
		tcSocketFd = connectClient("tcSocket");		// connessione con tc server
		printf("%s", "ECU-CLIENT: connected to tcSocket\n");

		bbwSocketFd = connectClient("bbwSocket");		// connessione con bbw server
		printf("ECU-CLIENT: connected to bbwSocket\n");

		sbwSocketFd = connectClient("sbwSocket");		// connessione con sbw server
		printf("%s", "ECU-CLIENT: connected to sbwSocket\n");

    	close(pipe_fwc_data[WRITE]);
		close(pipe_ecu_logger[READ]);

    	char data[MAX_DATA_SIZE];
		while(read(pipe_fwc_data[READ], data, MAX_DATA_SIZE)){
			processFwcData(data);
		}

		printf("CHIUSO MANAGER FCW\n");
		close(pipe_fwc_data[READ]);		// look: ha senso metterlo? non verrà mai eseguito
		//close(pipe_ecu_logger[WRITE]);
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
			decodeFfrData(data);
		}

		printf("CHIUSO MANAGER FFR\n");
		close(pipe_ffr_data[READ]);		// look: ha senso metterlo? non verrà mai eseguito
        exit(0);
    }
}

void bsDataManager(pid_t pidEcuClientBsManager){
    if(pidEcuClientBsManager < 0){
        perror("fork");
        exit(1);
    }
    if(pidEcuClientBsManager == 0) {
    	close(pipe_bs_data[WRITE]);

    	// look: devo riconoscere se all'interno del dato ci sono quel tipo di sequenze da testo progetto -> in tal caso kill a brake-by-wire
    	char data[MAX_DATA_SIZE];
		while(read(pipe_bs_data[READ], data, MAX_DATA_SIZE)){
			printf("BS MANAGER: leggo\n");
			decodeBsData(data);
		}

		printf("CHIUSO MANAGER BS\n");
		close(pipe_bs_data[READ]);		// look: ha senso metterlo? non verrà mai eseguito
        exit(0);
    }
}

void creaServers() {
	close(pipe_fwc_data[READ]);
	close(pipe_ffr_data[READ]);
	close(pipe_bs_data[READ]);

	creaServer(pidFwcEcuServer, fwcSocketFd, "fwcSocket");
	creaServer(pidFfrEcuServer, ffrSocketFd, "ffrSocket");
	creaServer(pidBsEcuServer, bsSocketFd, "bsSocket");

	/*close(pipe_fwc_data[WRITE]);
	close(pipe_ffr_data[WRITE]);
	close(pipe_bs_data[WRITE]);*/
}

void creaServer(pid_t pidEcuServer, int fdSocketSensore, char *nomeSocketSensore) {
	int clientFd;

	pidEcuServer = fork();
	if(pidEcuServer == 0) {
		fdSocketSensore = socket (AF_UNIX, SOCK_STREAM, DEFAULT_PROTOCOL);	// file descriptor socket sensori

		strcpy (serverUNIXAddress.sun_path, nomeSocketSensore); /* Set name */
		unlink (nomeSocketSensore);		/* Remove file if it already exists */
		bind (fdSocketSensore, serverSockAddrPtr, serverLen);/*Create file*/
		listen (fdSocketSensore, 1);	/* Maximum pending connection length */

		printf("ECU-SERVER %s: wait client\n", nomeSocketSensore);
		clientFd = accept (fdSocketSensore, clientSockAddrPtr, &clientLen);	// Accept a client connection - bloccante
		printf("ECU-SERVER %s: accept client\n", nomeSocketSensore);

		char data[MAX_DATA_SIZE];
		printf("ECU-SERVER %s: wait to read something\n", nomeSocketSensore);
		while (readSocket(clientFd, data)) {			// look: cosa ritorna read se socket vuoto? (so che ritorna 0 se END FILE)
			if(strcmp(nomeSocketSensore, "fwcSocket") == 0) {
				write(pipe_fwc_data[WRITE], data, strlen(data)+1);
			} else if(strcmp(nomeSocketSensore, "ffrSocket") == 0) {
				write(pipe_ffr_data[WRITE], data, strlen(data)+1);
			}else if(strcmp(nomeSocketSensore, "paSocket") == 0){
				printf("ECU-SERVER LEGGE DA PA SOCKET\n");
				decodePaData(data);
			} else if(strcmp(nomeSocketSensore, "bsSocket") == 0){
				write(pipe_bs_data[WRITE], data, strlen(data)+1);
			}
		}

		printf("ECU-SERVER %s: end to read socket PID = %d\n", nomeSocketSensore, getpid());

		close (clientFd); /* Close the socket */
		exit (0); /* Terminate */
	}
}

void processFwcData(char *data) {
	char command[20];

	strcpy(command, "NESSUN COMANDO");		// reset command
	if(strcmp(data, "DESTRA") == 0 || strcmp(data, "SINISTRA") == 0) {
		sprintf(command, "%s", data);		// copia data in command

		writeSocket(sbwSocketFd, command);				// scrivo a steer-by-wire
	} else if(strcmp(data, "PERICOLO") == 0) {
		printf("ECU-CLIENT: leggo PERICOLO\n");

		currentSpeed = 0;
		kill(pidBbw, SIGDANGER);	// invio segnale di pericolo a break by wire

		// look: ASPETTARE BBW CHE FINISCA DI SCRIVERE SU FILE?!

		kill(pidHmi, SIGDANGER);	// invio segnale di pericolo a HMI
		writeSocket(sbwSocketFd, data);
	} else {	// viene letto un numero
		int nextSpeed = atoi(data);
		int deltaSpeed;
        writeSocket(sbwSocketFd, command); // Se è un numero quello che leggo da frontcamera allora scrivo il valore
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
		// Scrivo ad ecu logger la velocità da aggiornare => currentSpeed variabile globale
		char updateSpeed[20];
		sprintf(updateSpeed, "UPDATE %d", currentSpeed);
		write(pipe_ecu_logger[WRITE], updateSpeed, strlen(updateSpeed)+1);

	}

	// scrivo a ecuLogger commando per OUTPUT
	if(strcmp(command, "NESSUN COMANDO") != 0) {			// controllo se la stringa command è diversa dalla stringa vuota
		printf("ProcessFwcData: (command != 'NESSUN COMANDO') => SCRIVO INCREMENTO/FRENO\n");
		write(pipe_ecu_logger[WRITE], command, strlen(command)+1);
	}
	//fflush(stdout);
}

void decodeFfrData(unsigned char *data) {
    unsigned char errValues[] = {0xA0, 0x0F, 0xB0, 0x72, 0x2F, 0xA8, 0x83, 0x59, 0xCE, 0x23};

    for(int i = 0; i < 10; i=i+2){
    	for(int j = 0; j < 24; j=j+2){
       		/*printf("CONFRONTO: %02X%02X - ", data[j],data[j+1]);
       		printf("%02X%02X\n", errValues[i],errValues[i+1]);*/

    		if(data[j] == errValues[i] && data[j+1] == errValues[i+1]){
				printf("FFR MANAGER: TROVATO HEX SIMILE - ERROR %02X\n", data[j]);
				kill(pidBbw, SIGDANGER); 	// invio segnale di pericolo a brake by wire

				// look: -----ATTENZIONE------- sembra che ad eseguire in conseguenza il progetto, ffr non apre i file per leggere roba

				kill(pidHmi, SIGDANGER);	// invio segnale di pericolo a HMI
    		}
    	}
    }
}

void decodeBsData(unsigned char *data) {
    unsigned char errValues[] = {0x41, 0x4E, 0x44, 0x52, 0x4C, 0x41, 0x42, 0x4F, 0x52, 0x41, 0x54, 0x4F, 0x83, 0x59, 0xA0, 0x1F, 0x52, 0x49, 0x51, 0x00};

    for(int i = 0; i < 20; i=i+2) {
		for(int j = 0; j < 8; j=j+2) {
       		/*printf("CONFRONTO: %02X%02X - ", data[j],data[j+1]);
       		printf("%02X%02X\n", errValues[i],errValues[i+1]);*/

    		if(data[j] == errValues[i] && data[j+1] == errValues[i+1]){
				printf("BS MANAGER: TROVATO HEX SIMILE - ERROR %02X\n", data[j]);
				kill(pidBbw, SIGDANGER); 	// invio segnale di pericolo a brake by wire

				// look: -----ATTENZIONE------- sembra che ad eseguire in conseguenza il progetto, ffr non apre i file per leggere roba

				kill(pidHmi, SIGDANGER);	// invio segnale di pericolo a HMI
    		}
		}
    }
}

void decodePaData(unsigned char *data) {
    unsigned char errValues[] = {0x17, 0x2A, 0xD6, 0x93, 0xBD, 0xD8, 0xFA, 0xEE, 0x43, 0x00};

    for(int i = 0; i < 10; i=i+2) {
		for(int j = 0; j < 4; j=j+2) {
       		printf("PA CONFRONTO: %02X%02X - ", data[j],data[j+1]);
       		printf("%02X%02X\n", errValues[i],errValues[i+1]);

    		if(data[j] == errValues[i] && data[j+1] == errValues[i+1]){
				printf("PA MANAGER: TROVATO HEX SIMILE - ERROR %02X\n", data[j]);
				kill(pidPa, SIGUSR2);
				// look: printf("CAZZOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO\n");
    		}
		}
    }
    printf("BELLA\n");
}

void ecuLogger() {
	FILE *ecuLogFd;
	char ecuCommand[MAX_DATA_SIZE];

	openFile("ECU.log","a", &ecuLogFd);			// "a": open file for appending
	//close(pipe_ecu_logger[WRITE]);

	int updatedSpeed;

	//while(read(pipe_ecu_logger[READ], ecuCommand, MAX_DATA_SIZE)){
	while(readPipe(pipe_ecu_logger[READ], ecuCommand)){
		//printf("ECU-LOGGER: LEGGO %s\n", ecuCommand);
		char receivedSpeed[20];
		sprintf(receivedSpeed, "%s", ecuCommand);
		if((updatedSpeed = getCurrentSpeed(receivedSpeed)) > -1){
			currentSpeed = updatedSpeed;
		} else {
			fprintf(ecuLogFd, "%s\n", ecuCommand);			// scrivo su ECU.log
			fflush(ecuLogFd);
		}
		//fflush(stdout);
	}

	close(pipe_ecu_logger[READ]);
	//fclose(ecuLogFd);
}


int getCurrentSpeed(char *fullCommand) {
	char *speed = strtok (fullCommand," ");

	if(strcmp(speed, "UPDATE") == 0){      // prima parola del comando == "UPDATE"
		speed = strtok (NULL, " ");
		return atoi(speed);
	} else {
		return -1;
	}
}

void parkingHandler() {
	signal(SIGPARK, endParkingHandler);			// signal per ricevere segnale fino parcheggio da parte di brake by wire

	bbwSocketFd = connectClient("bbwSocket");	// connessione server brake-by-wire

	char parkCommand[20];
	sprintf(parkCommand, "PARCHEGGIO %d", currentSpeed);
	writeSocket(bbwSocketFd, parkCommand);			// invio comando parcheggio a brake-by-wire

	write(pipe_ecu_logger[WRITE], parkCommand, strlen(parkCommand)+1);	// invio comando parcheggio a ECU-LOGGER	

	close(pipe_pa_data[READ]);
	creaServer(pidPaEcuServer, paSocketFd, "paSocket");			// creo ECU-SERVER <--> PARK ASSIST CLIENT

	// look: GUARDA PRIMO FACOLTATIVO PAG 4 => DOVREI KILLARE GLI ALTRI ATTUATORI tc, sbw
	//kill(pidTc, SIGTERM);
	//kill(pidSbw, SIGTERM);

	kill(pidFwc, SIGKILL);		//look: fare fin da subito? Altrimenti possono arrivarmi dei comandi per TC,BBW o SBW
}

void endParkingHandler() {
	signal(SIGPARK, endProgram);	

	// Arrivati a questo punto, elocità = 0, Central ECU attiva Park assist ultrasonic sensors e Surround view cameras.
	creaParkAssist();
}

void creaParkAssist() {
	char *argv[3];
	argv[0] = "./pa";
	argv[1] = startMode;
	argv[2] = NULL;

	pidPa = fork();
	creaComponente(pidPa, argv);
}

void endProgram() {
	kill(pidHmi, SIGPARK);		// hmi chiuderà tutti i processi attivi del programma
}


int readPipe(int pipeFd, char *data) {
	int n;
	do { /* Read characters until ’\0’ or end-of-input */
		n = read (pipeFd, data, 1); /* Read one character */
	} while (n > 0 && *data++ != '\0');

	return (n > 0);
}