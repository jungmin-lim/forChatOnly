#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <utmp.h>
#include <stdlib.h>

#define BUF_LENGTH 1024

int main(){
	FILE* fp;
	int i;
	char command[BUF_LENGTH], encrypt[BUF_LENGTH], decrypt[BUF_LENGTH];
	char readbuf[BUF_LENGTH];
	gets(command);
	for (i = 0; i < strlen(command); i++) {
		encrypt[i] = command[i] + 3;
	}
	puts(encrypt);
	for (i = 0; i < strlen(command); i++) {
		decrypt[i] = encrypt[i] + 3;
	}
	puts(decrypt);
	
	fp=popen(command, "r");
	while (fgets(readbuf, BUF_LENGTH, fp) != NULL) {
		printf("%s", readbuf);
	}
	
}
