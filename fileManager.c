#include <stdio.h>
#include <stdlib.h>
#include "fileManager.h"

void closeFile(FILE *fd) {        // look: preoccuparsi di chiudere file (?)
}

void openFile(char filename[], char mode[], FILE **filePointer) {
	*filePointer = fopen(filename, mode);
	if(*filePointer == NULL) {
		printf("Errore nell'apertura del file");
		printf("%s\n", filename);
		exit(1);
	}
}
