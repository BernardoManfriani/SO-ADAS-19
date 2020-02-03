#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>

#include<signal.h>

#include <sys/wait.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "../lib/socketManager.h"
#include "../lib/fileManager.h"

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

// ========================== PID ==========================
pid_t pidHmi, pidEcu;

// PID ATTUATORI
pid_t pidTc, pidBbw, pidSbw;

// PID SENSORI
pid_t pidFwc, pidFfr, pidPa, pidSvc;

// PID ECU-CLIENTS  - clienti del socket aperto con gli attuatori
pid_t pidEcuClientFwcManager, pidEcuClientFfrManager, pidEcuClientBsManager;

// PID ECU-SERVERS
pid_t pidFwcEcuServer, pidFfrEcuServer, pidPaEcuServer, pidSvcEcuServer, pidBsEcuServer;


// ========================== PIPE ==========================
// PIPE - da ECU-SERVER a ECU-CLIENT
int pipe_fwc_data[2], pipe_ffr_data[2], pipe_bs_data[2];

// PIPE per comunicazione con hmi
int pipe_ecu_logger[2];


// ========================== SOCKET ==========================
// FD SOCKET ATTUATORI
int tcSocketFd, bbwSocketFd, sbwSocketFd;

// FD SOCKET SENSORI
int fwcSocketFd, ffrSocketFd, paSocketFd, svcSocketFd, bsSocketFd;


// ========================== Strutture dati server-client ==========================
int serverLen, clientLen;
struct sockaddr_un serverUNIXAddress; /*Server address */
struct sockaddr* serverSockAddrPtr; /*Ptr to server address*/
struct sockaddr_un clientUNIXAddress; /*Client address */
struct sockaddr* clientSockAddrPtr;/*Ptr to client address*/


void startEcuHandler();
void parkingHandler();
void endParkingHandler();
void init();
void start();

void creaComponente(pid_t pidSens, char *argv[]);

void creaSensori();

void creaServers();
void creaServer(char *sensorName, pid_t pidEcuServer, int fdSocketSensore, char *nomeSocketSensore);

void creaParkAssist();
void creaSurroundViewCameras();

void creaEcuClients();
void fwcDataManager(pid_t );
void ffrDataManager(pid_t );
void bsDataManager(pid_t );

void creaAttuatori();

void processFwcData(char *data);
void decodeFfrData(unsigned char *data);
void decodeBsData(unsigned char *data);
void decodePaData(unsigned char *data);
void decodeSvcData(unsigned char *data);

void manageDanger();

void ecuLogger();
int getCurrentSpeed(char *);

void endProgram();

int readPipe (int pipeFd, char *data);

int main(int argc, char *argv[]) {
	startMode = argv[1];		// startMode = modalità di avvio
	pidHmi = getppid();
	pidEcu = getpid();

	signal(SIGSTART, startEcuHandler);

	init();			// inizializzo pipe e strutture dati per socket
	pause();

	start();

	return 0;
}

void init() {		// inizializza le pipe e struttura dati socket
	currentSpeed = 0;

	pipe(pipe_fwc_data);
	pipe(pipe_ffr_data);
	pipe(pipe_bs_data);
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

	creaEcuClients();		// ECU-CLIENTS

	creaServers();			// ECU-SERVER

	ecuLogger();
}

void startEcuHandler() {
	signal(SIGPARK, parkingHandler);	// signal triggerata dopo inserimento comando PARCHEGGIO

	signal(SIGINT, endProgram);	// signal triggerata dopo inserimento comando PARCHEGGIO
}

void creaSensori() {
	char *argv[3];
	argv[1] = startMode;
	argv[2] = NULL;

	argv[0] = "./fwc";
	pidFwc = fork();
	creaComponente(pidFwc, argv);			// Sensore front windshield camera*/

	argv[0] = "./ffr";
	pidFfr = fork();
	creaComponente(pidFfr, argv);			// Sensore forward facing radar*/
}

void creaAttuatori() {
	char *argv[3];
	argv[1] = startMode;
	argv[2] = NULL;

    argv[0] = "./tc";
    pidTc = fork();
	creaComponente(pidTc, argv);			// Attuatore throttle control*/

	argv[0] = "./bbw";
    pidBbw = fork();
	creaComponente(pidBbw, argv);			// Attuatore brake by wire*/

    argv[0] = "./sbw";
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

void creaEcuClients() {
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
		tcSocketFd = connectClient("tcSocket");			// connessione con tc server

		bbwSocketFd = connectClient("bbwSocket");		// connessione con bbw server

		sbwSocketFd = connectClient("sbwSocket");		// connessione con sbw server

    	close(pipe_fwc_data[WRITE]);
		close(pipe_ecu_logger[READ]);

    	char data[MAX_DATA_SIZE];
		while(read(pipe_fwc_data[READ], data, MAX_DATA_SIZE)){
			processFwcData(data);
		}

    }
}

void ffrDataManager(pid_t pidEcuClientFfrManager){
    if(pidEcuClientFfrManager < 0){
        perror("fork");
        exit(1);
    }
    if(pidEcuClientFfrManager == 0) {
    	close(pipe_ffr_data[WRITE]);

    	char data[MAX_DATA_SIZE];
		while(read(pipe_ffr_data[READ], data, MAX_DATA_SIZE)){
			decodeFfrData(data);
		}
    }
}

void bsDataManager(pid_t pidEcuClientBsManager){
    if(pidEcuClientBsManager < 0){
        perror("fork");
        exit(1);
    }
    if(pidEcuClientBsManager == 0) {
    	close(pipe_bs_data[WRITE]);

    	char data[MAX_DATA_SIZE];
		while(read(pipe_bs_data[READ], data, MAX_DATA_SIZE)){
			decodeBsData(data);
		}
    }
}

void creaServers() {
	close(pipe_fwc_data[READ]);
	close(pipe_ffr_data[READ]);
	close(pipe_bs_data[READ]);

	creaServer("front-windshield-camera", pidFwcEcuServer, fwcSocketFd, "fwcSocket");
	creaServer("forward-facing-radar", pidFfrEcuServer, ffrSocketFd, "ffrSocket");
	creaServer("blind-spot", pidBsEcuServer, bsSocketFd, "bsSocket");
}

void creaServer(char *sensorName, pid_t pidEcuServer, int fdSocketSensore, char *nomeSocketSensore) {
	int clientFd;

	pidEcuServer = fork();
	if(pidEcuServer == 0) {
		fdSocketSensore = socket (AF_UNIX, SOCK_STREAM, DEFAULT_PROTOCOL);	// file descriptor socket sensori

		strcpy (serverUNIXAddress.sun_path, nomeSocketSensore); /* Set name */
		unlink (nomeSocketSensore);		/* Remove file if it already exists */
		bind (fdSocketSensore, serverSockAddrPtr, serverLen);/*Create file*/
		listen (fdSocketSensore, 1);	/* Maximum pending connection length */

		clientFd = accept (fdSocketSensore, clientSockAddrPtr, &clientLen);	// Accept a client connection
		printf("SENSORE %s: connected\n", sensorName);

		char data[MAX_DATA_SIZE];
		while (readSocket(clientFd, data)) {
			if(strcmp(nomeSocketSensore, "fwcSocket") == 0) {
				write(pipe_fwc_data[WRITE], data, strlen(data)+1);
			} else if(strcmp(nomeSocketSensore, "ffrSocket") == 0) {
				write(pipe_ffr_data[WRITE], data, strlen(data)+1);
			} else if(strcmp(nomeSocketSensore, "bsSocket") == 0){
				write(pipe_bs_data[WRITE], data, strlen(data)+1);
			} else if(strcmp(nomeSocketSensore, "paSocket") == 0){
				decodePaData(data);
			} else if(strcmp(nomeSocketSensore, "svcSocket") == 0){
				decodeSvcData(data);
			} 
		};

		close (clientFd); /* Close the socket */
		exit (0); /* Terminate */
	}
}

void ecuLogger() {
	FILE *ecuLogFd;
	openFile("../log/ECU.log","a", &ecuLogFd);			// "a": open file for appending

	char ecuCommand[MAX_DATA_SIZE];
	int updatedSpeed;
	while(readPipe(pipe_ecu_logger[READ], ecuCommand)){
		char receivedSpeed[20];
		sprintf(receivedSpeed, "%s", ecuCommand);

		if((updatedSpeed = getCurrentSpeed(receivedSpeed)) > -1){		// E' stato letto un numero, accelero/freno
			currentSpeed = updatedSpeed;
		} else {
			fprintf(ecuLogFd, "%s\n", ecuCommand);			// scrivo su ECU.log
			fflush(ecuLogFd);
		}
	}
}

										// ====== PROCESS SENSOR DATA ====== //
void processFwcData(char *data) {
	char command[20];
	strcpy(command, "NESSUN COMANDO");		// init command

	if(strcmp(data, "DESTRA") == 0 || strcmp(data, "SINISTRA") == 0) {
		sprintf(command, "%s", data);		// command = data
		writeSocket(sbwSocketFd, command);				// scrivo a steer-by-wire

	} else if(strcmp(data, "PERICOLO") == 0) {
		printf("!!! PERICOLO RILEVATO !!!\n");

		currentSpeed = 0;
		manageDanger();

	} else if(strcmp(data, "") == 0) {	// fwc arriva a fine file frontcamera.data
		kill(pidHmi, SIGPARK); 

	} else {	// viene letto un numero
		int nextSpeed = atoi(data);
		int deltaSpeed;
        if(nextSpeed > currentSpeed) {
			deltaSpeed = nextSpeed - currentSpeed;

			currentSpeed = nextSpeed;		// aggiorno velocità

			sprintf(command, "INCREMENTO %d", deltaSpeed);	// command = "INCREMENTO X"
			writeSocket(tcSocketFd, command);	// scrivo a throttle-control

		} else if (nextSpeed < currentSpeed) {
			deltaSpeed = currentSpeed - nextSpeed;

			currentSpeed = nextSpeed;

			sprintf(command, "FRENO %d", deltaSpeed);	// command = "FRENO X"
			writeSocket(bbwSocketFd, command);	// scrivo a brake-by-wire
		}

		// Invio a processo ecu logger la velocità da aggiornare
		char updateSpeed[20];
		sprintf(updateSpeed, "UPDATE %d", currentSpeed);
		write(pipe_ecu_logger[WRITE], updateSpeed, strlen(updateSpeed)+1);

	}

	// Invio a ecuLogger commando per OUTPUT
	if(strcmp(command, "NESSUN COMANDO") != 0) {		// controllo se la stringa command è diversa dalla stringa vuota
		write(pipe_ecu_logger[WRITE], command, strlen(command)+1);
	}
}

void decodeFfrData(unsigned char *data) {
    unsigned char errValues[] = {0xA0, 0x0F, 0xB0, 0x72, 0x2F, 0xA8, 0x83, 0x59, 0xCE, 0x23};

    for(int i = 0; i < 10; i=i+2){
    	for(int j = 0; j < 24; j=j+2){

    		if(data[j] == errValues[i] && data[j+1] == errValues[i+1]){
				printf("ERROR: forward-facing-radar sequenza maligna trovata\n");
				manageDanger();
    		}
    	}
    }
}

void decodeBsData(unsigned char *data) {
    unsigned char errValues[] = {0x41, 0x4E, 0x44, 0x52, 0x4C, 0x41, 0x42, 0x4F, 0x52, 0x41, 0x54, 0x4F, 0x83, 0x59, 0xA0, 0x1F, 0x52, 0x49, 0x51, 0x00};

    for(int i = 0; i < 20; i=i+2) {
		for(int j = 0; j < 8; j=j+2) {

    		if(data[j] == errValues[i] && data[j+1] == errValues[i+1]){
				printf("ERROR: blind-spot sequenza maligna trovata\n");
				manageDanger();
    		}
		}
    }
}

void decodePaData(unsigned char *data) {
    unsigned char errValues[] = {0x17, 0x2A, 0xD6, 0x93, 0xBD, 0xD8, 0xFA, 0xEE, 0x43, 0x00};

    for(int i = 0; i < 10; i=i+2) {
		for(int j = 0; j < 4; j=j+2) {

    		if(data[j] == errValues[i] && data[j+1] == errValues[i+1]){
				printf("ERROR: park-assist sequenza maligna trovata\n");
				kill(pidPa, SIGINT);		// reset procedura di parcheggio
    		}
		}
    }
}

void decodeSvcData(unsigned char *data) {
    unsigned char errValues[] = {0x17, 0x2A, 0xD6, 0x93, 0xBD, 0xD8, 0xFA, 0xEE, 0x43, 0x00};

    for(int i = 0; i < 10; i=i+2) {
		for(int j = 0; j < 4; j=j+2) {
    		if(data[j] == errValues[i] && data[j+1] == errValues[i+1]){
				printf("ERROR: surround-view-cameras sequenza maligna trovata\n");
				kill(pidPa, SIGINT);		// reset procedura di parcheggio
    		}
		}
    }
}

void manageDanger() {
	kill(pidBbw, SIGDANGER);	// invio segnale di pericolo a break by wire

	kill(pidFwc, SIGTERM);	// invio segnale di pericolo a FWC
	sleep(1);

	kill(pidHmi, SIGDANGER);	// invio segnale di pericolo a HMI
	printf("Veicolo arrestato\nPremi INIZIO per ripartire\n\n");
}

										// ====== PARKING ROUTINES ====== //
void parkingHandler() {
	signal(SIGPARK, endParkingHandler);			// signal per ricevere segnale fino parcheggio da parte di brake by wire

	bbwSocketFd = connectClient("bbwSocket");	// connessione server brake-by-wire

	char parkCommand[20];
	sprintf(parkCommand, "PARCHEGGIO %d", currentSpeed);
	writeSocket(bbwSocketFd, parkCommand);			// invio comando parcheggio a brake-by-wire

	write(pipe_ecu_logger[WRITE], parkCommand, strlen(parkCommand)+1);	// invio comando parcheggio a ECU-LOGGER	


	creaParkAssist();
    kill(pidPa, SIGSTOP);		// Avvierò componente, nel momento in cui velocità == 0

	creaSurroundViewCameras();
    kill(pidSvc, SIGSTOP);		// Avvierò componente, nel momento in cui velocità == 0

	creaServer("park-assist", pidPaEcuServer, paSocketFd, "paSocket");			// creo ECU-SERVER <--> PARK ASSIST CLIENT
	creaServer("surround-view-cameras", pidSvcEcuServer, svcSocketFd, "svcSocket");		// creo ECU-SERVER <--> SURROUND-VIEW-CAMERAS CLIENT

	// Termino componenti non utili al parcheggio
	kill(pidTc, SIGTERM);
	kill(pidSbw, SIGTERM);

	kill(pidFfr, SIGTERM);
	kill(pidFwc, SIGTERM);
}

void endParkingHandler() {
	signal(SIGPARK, endProgram);
	printf("\nparcheggio in corso...\n");

	// Arrivati a questo punto, velocità = 0, Central ECU attiva Park assist ultrasonic sensors e Surround view cameras
    kill(pidPa, SIGCONT);
    kill(pidSvc, SIGCONT);
}

void creaParkAssist() {
	char *argv[3];
	argv[0] = "./pa";
	argv[1] = startMode;
	argv[2] = NULL;

	pidPa = fork();
	creaComponente(pidPa, argv);
}

void creaSurroundViewCameras() {
	char *argv[3];
	argv[0] = "./svc";
	argv[1] = startMode;
	argv[2] = NULL;

	pidSvc = fork();
	creaComponente(pidSvc, argv);
}

void endProgram() {
	kill(pidSvc, SIGTERM);		// park-assist termina => termina anche svc

	kill(pidHmi, SIGPARK);		// hmi chiuderà tutti i processi relativi a questa simulazione
}

										// ====== UTILITY ====== //
int readPipe(int pipeFd, char *data) {
	int n;
	do { /* Read characters until ’\0’ or end-of-input */
		n = read (pipeFd, data, 1); /* Read one character */
	} while (n > 0 && *data++ != '\0');

	return (n > 0);
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