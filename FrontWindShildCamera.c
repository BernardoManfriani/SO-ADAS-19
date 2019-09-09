#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>


void openFile(char filename[], char mode[], FILE **filePointer);
void readFile(FILE* fd,FILE* fc);

int main() {
	
	FILE *fd;
	FILE *fc;

    openFile("frontCamera.data","r", &fd);
  	
  	
	/* legge e stampa ogni riga */

    
  	
  	openFile("frontCamera.log","w", &fc);
  	//fc=fopen("frontCamera.log", "w");
    readFile(fd,fc);
  	

	/* chiude il file */

  	fclose(fd);
  	fclose(fc);

  	return 0;
}

void readFile(FILE* fd, FILE* fc){
	char buf[200];
  	char *res;
  	while(1) {
   		res=fgets(buf, 200, fd);

   		if(res==NULL )
    		break;

 		//sleep(10);
    	fprintf(fc,"%s",buf);
  	}

	
}
void openFile(char filename[], char mode[], FILE **filePointer) {
	*filePointer = fopen(filename, mode);
	if (*filePointer == NULL) {
		printf("Errore nell'apertura del file");
		exit(1);
	}
}