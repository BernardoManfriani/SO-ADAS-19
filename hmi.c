#include<stdio.h>
#include<stdlib.h>
#include <string.h>
#include<unistd.h>
#include<sys/types.h>

pid_t pid;

void start();

int main(int argc, char *argv[]) {
    if (argc < 2 || (strcmp(argv[1], "NORMALE") != 0 && strcmp(argv[1], "ARTIFICIALE") != 0)) {
		printf("Specifica se vuoi un avvio NORMALE o ARTIFICIALE\n");
		exit(1);
	}
    pid = fork();
    if(pid < 0) {
        perror("fork");
        exit(1);
    }
    if(pid == 0) {  // ECU child process
        setpgid(0,0);
        argv[0] = "./ecu";
        execv(argv[0], argv);
    } 
    else { // HMI parent
        start();
    }

    return 0;
}

void start(){
	char input[30];
	short int started = 0;
    printf("Benvenuto nel simulatore di sistemi di guida autonoma. \nDigita INIZIO per avviare il veicolo,\no digita PARCHEGGIO per avviare la procedura di parcheggio e concludere il percorso.\n\n");	
	while(1) {
		if(fgets(input, 30, stdin) != NULL){
			if((started) == 0) {
				if(strcmp(input, "INIZIO\n") == 0) {
					printf("Veicolo avviato\n");
					//kill(ecuPid, SIGSTART);
					started = 1;
				} else if (strcmp(input, "PARCHEGGIO\n") == 0) {
					printf("Prima di poter parcheggiare devi avviare il veicolo.\nDigita INIZIO per avviare il veicolo.\n\n");
				} else {
					printf("Comando non ammesso.\n\n");
				}
			} else {    // Una volta avviata la macchina, concesso solo parcheggio
				if(strcmp(input, "PARCHEGGIO\n") == 0) {
					printf("Sto fermando il veicolo...\n");
					//kill(ecuPid, SIGPARK);
					started = 0;
				} else {
					printf("Comando non ammesso. \nDigita PARCHEGGIO per parcheggiare il veicolo\n\n");
				}
			}
		}
	}
	return;
}