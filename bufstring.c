#include <stdio.h>
#include <string.h>
#include "bufstring.h"

void insert_char(char* msg, int pos, int ch){
	char buf[BUFSIZ];
	sprintf(buf, "%.*s%c%.*s", pos, msg, ch, (int)strlen(msg) - pos, msg + pos);
	sprintf(msg, "%s", buf);
}

void erase_char(char* msg, int pos){
	char buf[BUFSIZ];
	sprintf(buf, "%.*s%.*s", pos, msg, (int)strlen(msg) - pos - 1, msg + pos + 1);
	sprintf(msg, "%s", buf);
}
