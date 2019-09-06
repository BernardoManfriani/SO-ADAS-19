#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

pid_t ecuPid;

static char *sub_process_name = "./ecu";

void start();

void parking();

void command();

void hello();


int main(int argc, char *argv[]){
	char v[20];
    char *comando=v;

 	if(strcmp(argv[1],"NORMALE")==0){
 		printf("Avvio normale \n");

 	}
 	if(strcmp(argv[1],"ARTIFICIALE\n")==0){
 		printf("Avvio artificiale");
 	}

 	ecuPid=fork();

 	if(ecuPid<0){
 		perror("fork");
 		return 1;
 	}
 	if(ecuPid==0){
 		setpgid(0,0);
 		argv[0]= sub_process_name;
 		execv(argv[0],argv);
 	}
 	else{
 		 hello(comando);
 	}


   
}     

void hello(char *comando[]){
	printf("Ciao per iniziare digita INIZIO per parcehggiare e concludere la procedura digita PARCHEGGIO\n");
	printf("Inserisci comando da eseguire: ");
    scanf("%s", comando);
    printf("\n");
    
    command(comando);
}

void command(char comando[]){
	
	if(strcmp(comando, "INIZIO")==0){
		start();
	}
	if(strcmp(comando, "PARCHEGGIO")==0){
		parking();
	}
	
}


void start(){
    printf("\nInizia a guidare\n");
    
}

void parking(){	
    printf("\n Sto parcheggiando\n");
}



    
