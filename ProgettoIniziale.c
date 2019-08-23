#include <stdio.h>

void inizio();

void parcheggio();


void main(){
    char   comando;
     
    printf("Inserisci comando da eseguire: ");
    scanf("%c" , &comando);
   
    if(comando=='p') {
        parcheggio();
    }
    if(comando=='i'){
        inizio();
    } 
}       

void inizio(){
    printf("\nInizia a guidare\n");
}

void parcheggio(){
    printf("\nParcheggia\n");
}

    
