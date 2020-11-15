#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 100
#define NAME_SIZE 20
#define ESC 27

void request_grouplist(int sock);

void* send_msg(void *arg);
void* recv_msg(void *arg);
void error_handling(char *msg);

char name[NAME_SIZE]="[DEFAULT]";
char msg[BUF_SIZE];

int main(int argc, char *argv[]){
	int sock;
	struct sockaddr_in serv_addr;
	pthread_t snd_thread, rcv_thread;
	void * thread_return;
	char buf[BUF_SIZE];
	int recv_len;

	if(argc!=3) {
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

	sock=socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_addr.sin_port=htons(atoi(argv[2]));

	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
		error_handling("connect() error");

	while(1){
		request_grouplist(sock);
		printf("end\n");
		scanf("%s", buf);
		write(sock, buf, strlen(buf) + 1);

		recv_len = read(sock, buf, BUF_SIZE);

		if(strncmp(buf, "OK", 2) == 0) {
			buf[recv_len] = '\0';
			printf("%s", buf);
			break;
		}
	}

	pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);
	pthread_join(snd_thread, &thread_return);
	
	pthread_join(rcv_thread, &thread_return);
	close(sock);  
	return 0;
}

void request_grouplist(int sock){
	char buf[BUF_SIZE];
	int str_len;
	int isRunning = 1;
	while(isRunning){
		str_len = read(sock, buf, BUF_SIZE-1);
		if(str_len == 0 || str_len == -1)
			break;

		if(buf[str_len-1] == (char)ESC){
			buf[str_len-1] = '\0';
			isRunning = 0;
		}
		else
			buf[str_len] = 0;
		fputs(buf, stdout);
	}
}

void* send_msg(void *arg) {
	int sock = *((int*)arg);
	char name_msg[NAME_SIZE+BUF_SIZE];
	while(1) {
		scanf("%s", msg);
		if(!strcmp(msg,"q\n") || !strcmp(msg,"Q\n")) {
			close(sock);
			exit(0);
		}
		sprintf(name_msg,"%s %s", name, msg);
		write(sock, name_msg, strlen(name_msg));
	}
	return NULL;
}

void* recv_msg(void *arg) {
	int sock = *((int*)arg);
	char name_msg[BUF_SIZE];
	int str_len;
	while(1) {
		str_len=read(sock, name_msg, NAME_SIZE+BUF_SIZE-1);
		if(str_len == 0 || str_len == -1) 
			return (void*)-1;
		name_msg[str_len]=0;
		fputs(name_msg, stdout);
	}
	return NULL;
}

void error_handling(char *msg) {
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
