#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include<signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>


#include <fcntl.h>

#define DEFAULT_PROTOCOL 0
#define READ 0
#define WRITE 1


int status;	//pipe status
int pipeFd[2]; //pipe array
pid_t pid;
FILE *fileLog;

int deltaSpeed;

void createServer();
int readFromSocket (int );
int readLines (int x, char *y);
void manageSocketData(char *data);
void writeLog();

void lettorePipe();

// void sigTermHandler();

void openFile(char filename[], char mode[], FILE **filePointer);

int main() {
    printf("%s\n", "---- throttle control attivo --------");

	status = pipe(pipeFd);
	if(status != 0) {
		printf("Pipe error\n");
		exit(1);
	}
	// fcntl(pipeFd[READ], F_SETFL, O_NONBLOCK);	//rende la read non bloccante

	pid = fork();
	if(pid == 0) {			// child process writer on throttle.log file
		printf("\nchild process writer on throttle.log file RUNNING\n");
		close(pipeFd[WRITE]);
		writeLog();
		close(pipeFd[READ]);
		return 0;
	} else {				// father process listener on socket
		printf("\nfather process listener on socket RUNNING\n");
		// signal(SIGTERM, sigTermHandler); //NON SO COSA FA - PUNTO AVANZATO PROGETTO
		close(pipeFd[READ]); 
        createServer();
		close(pipeFd[WRITE]);		// NECESSARIO ?
	}
	printf("FINE\n");
	exit(0);
}

void createServer() {
    int serverFd, clientFd, serverLen, clientLen;
    struct sockaddr_un serverUNIXAddress; /*Server address */
    struct sockaddr_un clientUNIXAddress; /*Client address */
    struct sockaddr* serverSockAddrPtr; /*Ptr to server address*/
    struct sockaddr* clientSockAddrPtr;/*Ptr to client address*/

    /* Ignore death-of-child signals to prevent zombies */
    signal (SIGCHLD, SIG_IGN);

    serverSockAddrPtr = (struct sockaddr*) &serverUNIXAddress;
    serverLen = sizeof (serverUNIXAddress);
    clientSockAddrPtr = (struct sockaddr*) &clientUNIXAddress;
    clientLen = sizeof (clientUNIXAddress);
    serverFd = socket (AF_UNIX, SOCK_STREAM, DEFAULT_PROTOCOL);

    serverUNIXAddress.sun_family = AF_UNIX; /* Set domain type */
    strcpy (serverUNIXAddress.sun_path, "tcSocket"); /* Set name */
    unlink ("tcSocket"); /* Remove file if it already exists */
    bind (serverFd, serverSockAddrPtr, serverLen);/*Create file*/
    listen (serverFd, 5); /* Maximum pending connection length */

    while (1) {/* Loop forever */ /* Accept a client connection */
        clientFd = accept (serverFd, clientSockAddrPtr, &clientLen);

        char data[30];

        if(fork () == 0) { /* Create child read ECU client */
			printf("aspetto di leggere qualcosa da socket\n");
            while(readLines(clientFd, data)) {
                manageSocketData(data);
            }

            close (clientFd); /* Close the socket */
            exit (0); /* Terminate */
        } else {
            close (clientFd); /* Close the client descriptor */
            exit (0);
        }
    }
}

int readFromSocket(int fd) {
    char str[100];
    while(readLines (fd, str)); /* Read lines until end-of-input */
    printf("%s\n", str);
}

int readLines(int fd, char *str) {
	int n;
	do { /* Read characters until ’\0’ or end-of-input */
		n = read(fd, str, 1); /* Read one character */
	} while(n > 0 && *str++ != '\0');
    return (n > 0);
}

void manageSocketData(char *data) {
    printf("LEGGO DA SOCKET = %s -> SCRIVO SU PIPE\n", data);
	int figa = write(pipeFd[WRITE], data, 30);     // write on pipe
	printf("\n%d\n", figa);
}

void writeLog() {
	openFile("throttle.log", "w", &fileLog);

	int bytesRead;
	char socketData [30];
	// char *socketData;		// ----------------- PERCHE CON QUESTO NON FUNZIONA -----------------	//
	while(1) {
		bytesRead = read (pipeFd[READ], socketData, 30);
		if(bytesRead != 0){
			printf ("Read %d bytes: %s\n", bytesRead, socketData);
		    fprintf(fileLog, socketData);
			fflush(fileLog);		// SE COMMENTO NON SCRIVE SU FILE-- CAZZO PERCHEEEEE TROIO
		}
		sleep(1);
	}
	
}

void openFile(char filename[], char mode[], FILE **filePointer) {
	*filePointer = fopen(filename, mode);
	if(*filePointer == NULL) {
		printf("Errore nell'apertura del file");
		exit(1);
	}
}


void sigTermHandler() {
	// kill(pid, SIGTERM);
	exit(0);
	
}