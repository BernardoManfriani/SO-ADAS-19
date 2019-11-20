#include <stdio.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

int main(){
	int i=0;
	while(1){
		sleep(1);
		printf("\nI'm park assist \n");
		i++;
		if(i>4){
			break;
		}
	}
}