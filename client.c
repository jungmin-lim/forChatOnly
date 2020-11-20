#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 100
#define NAME_SIZE 20
#define MAX_GRP 32
#define ESC 27

void receive_grouplist(int sock);

void* send_msg(void *arg);
void* recv_msg(void *arg);
void error_handling(char *msg);

char name[NAME_SIZE]="[DEFAULT]";
char msg[BUF_SIZE+1];
int str_len;

int main(int argc, char *argv[]){
    int sock, group_id;
    struct sockaddr_in serv_addr;
    pthread_t snd_thread, rcv_thread;
    void * thread_return;
    char buf[BUF_SIZE];
    int recv_len, pos;

    if(argc!=3) {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sock=socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
    serv_addr.sin_port=htons(atoi(argv[2]));

    // connect server
    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
        error_handling("connect() error");

    // receive group list
    while(1){
        receive_grouplist(sock);

		fprintf(stdout, "Select group to join: ");
		fflush(stdout);
		scanf(" %s", buf);

		// create new group
		if(buf[0] == 'N' || buf[0] == 'n'){
			sprintf(msg, "%s", buf);

			fprintf(stdout, "Input group name to create: ");
			fflush(stdout);
			scanf(" %s", buf);
			strcat(msg, buf);
			// send selection & group name to create
			write(sock, msg, strlen(msg));
			str_len = read(sock, msg, sizeof(msg));
			msg[str_len] = '\0';
			if(str_len <= 0){
				error_handling("connection lost!");
			}

			strncpy(buf, msg, 2);
			buf[2] = '\0';
			// successfully created group
			if(!strcmp(buf, "OK")){
				fprintf(stdout, "Input your name to use: ");
				fflush(stdout);
				scanf(" %s", msg);

				// send user name
				write(sock, msg, strlen(msg));
				break;
			}
			// fail to create group
			else{
				error_handling(msg);
			}
		}
		// refresh group list
		else if (buf[0] == 'R' || buf[0] == 'r'){
			strcpy(msg, buf);
			write(sock, msg, strlen(msg));
			str_len = read(sock, msg, sizeof(msg));
			if(str_len <= 0){
				error_handling("connection lost!");
			}
		}
		// try joining existing group
		else{
			sscanf(buf, "%d", &group_id);
			if(0 <= group_id && group_id < MAX_GRP){
				msg[0] = '\0';
				strcpy(msg, buf);
				write(sock, msg, strlen(msg));
				str_len = read(sock, msg, sizeof(msg));
				if(str_len <= 0){
					error_handling("connection lost!");
				}

				strncpy(buf, msg, 2);
				buf[2] = '\0';

				if(!strcmp(buf, "OK")){
					printf("Success to connect group!\n");
					printf("Enter your name: ");
					fflush(stdout);

					scanf(" %s", msg);
					strncpy(name, msg, strlen(msg));
					write(sock, msg, strlen(msg));

					sprintf(msg, "%s is joined!", name);
					write(sock, msg, strlen(msg));
					break;
				}
				else{
					printf("Fail to connect group\n");
				}
			}
			else{
				printf("Unkwon command\n");
			}

		}
	}

	pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);
	pthread_join(snd_thread, &thread_return);
	
	pthread_join(rcv_thread, &thread_return);
	close(sock);  
	return 0;
}

void receive_grouplist(int sock){
    char buf[BUF_SIZE];
    int str_len;
    int isRunning = 1;
    while(isRunning){
        str_len = read(sock, buf, BUF_SIZE-1);
        if(str_len <= 0)
            break;

        if(buf[str_len-1] == (char)ESC){
            buf[str_len-1] = '\0';
            isRunning = 0;
        }
        else{
            buf[str_len] = 0;
        }
        fprintf(stdout, "%s", buf);
		fflush(stdout);
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
		write(sock, msg, strlen(msg)+1);
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
		name_msg[str_len]='\0';
		fputs(name_msg, stdout);
		fflush(stdout);
	}
	return NULL;
}

void error_handling(char *msg) {
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}