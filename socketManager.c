#include<unistd.h>
#include<string.h>
#include "socketManager.h"

int readSocket(int fd, char *str) {
	int n;
	do { /* Read characters until ’\0’ or end-of-input */
		n = read(fd, str, 1); /* Read one character */
	} while (n > 0 && *str++ != '\0');
	return (n > 0);
}

void writeSocket (int socketFd, char *data) {
    write(socketFd, data, strlen (data) + 1);
}