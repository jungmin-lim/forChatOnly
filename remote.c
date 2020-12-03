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
	char command[BUF_LENGTH];
	char readbuf[BUF_LENGTH];
	scanf("%s", command);
	fp=popen(command, "r");
	while (fgets(readbuf, BUF_LENGTH, fp) != NULL) {
		printf("%s", readbuf);
	}
	
}
