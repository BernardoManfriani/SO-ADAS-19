#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <sys/types.h>
#include <unistd.h>

#include<signal.h>

#define SIGSTART SIGUSR1
#define SIGPARK SIGUSR1
#define SIGDANGER SIGUSR2

#define READ 0
#define WRITE 1

pid_t ecuPid;
short int started = 0;
char **startMode;

void start();
void dangerHandler();

int main(int argc, char *argv[]) {
	startMode = argv;
    if (argc < 2 || (strcmp(argv[1], "NORMALE") != 0 && strcmp(argv[1], "ARTIFICIALE") != 0)) {
		printf("Specifica se vuoi un avvio NORMALE o ARTIFICIALE\n");
		exit(1);
	}

    ecuPid = fork();
    if(ecuPid < 0) {
        perror("fork");
        exit(1);
    }
    if(ecuPid == 0) {  			// ECU child process
        setpgid(0,0);			// crea gruppo processi con "leader gruppo" ./ecu - con una kill all child processes are killed
        argv[0] = "./ecu";
        execv(argv[0], argv);	// argv[] = ["./ecu", "NORMALE o ARTIFICIALE"]

        exit(0);
    }
    else { // HMI parent
    	signal(SIGDANGER, dangerHandler);
        start();
				printf("Wait end ecu\n");
        wait(NULL);			// aspetta finisca il processo figlio
        printf("\nHMI CONCLUSA - passo e chiudo\n");
    }

    return 0;
}

void start(){
	char input[30];
    printf("Benvenuto nel simulatore di sistemi di guida autonoma. \nDigita INIZIO per avviare il veicolo,\no digita PARCHEGGIO per avviare la procedura di parcheggio e concludere il percorso.\n\n");
	while(1) {
		if(fgets(input, 30, stdin) != NULL){
			if((started) == 0) {
				if(strcmp(input, "INIZIO\n") == 0) {
					printf("Veicolo avviato\n");
					kill(ecuPid, SIGSTART);					// look: ECU avviata solamente una volta scritto INIZIO
					started = 1;
					//break;									// look: per ora così, almeno si termina il processo hmi
				} else if (strcmp(input, "PARCHEGGIO\n") == 0) {
					printf("Prima di poter parcheggiare devi avviare il veicolo.\nDigita INIZIO per avviare il veicolo.\n\n");
				} else {
					printf("Comando non ammesso.\n\n");
				}
			} /*else {    // Una volta avviata la macchina, concesso solo parcheggio
				if(strcmp(input, "PARCHEGGIO\n") == 0) {
					printf("Sto fermando il veicolo...\n");
					kill(ecuPid, SIGPARK);	// durante parcheggio kill ecu process
					started = 0;
				} else {
					printf("Comando non ammesso. \nDigita PARCHEGGIO per parcheggiare il veicolo\n\n");
				}
			}*/
		}
	}
	return;
}

void dangerHandler() {
	signal(SIGDANGER, dangerHandler);
	kill(-ecuPid, SIGKILL);		// -ecuPid in modo da riferirsi all'intero gruppo

	ecuPid = fork();
    if(ecuPid < 0) {
        perror("fork");
        exit(1);
    }
    if(ecuPid == 0) {  			// ECU child process
        setpgid(0,0);
        startMode[0] = "./ecu";
        execv(startMode[0], startMode);	// argv[] = ["./ecu", "NORMALE o ARTIFICIALE"]
        exit(0);
    }

	printf("Macchina arrestata\nPremi INIZIO per ripartire\n\n");
	started = 0;
}
