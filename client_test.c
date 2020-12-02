#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUFSZ 1024
#define NAMESZ 64
#define ESC (char)27

void error_handling(char*);
void receive_group_list(int, char*);
int receive_msg(int, char*);
void* send_msg(void*);
void* recv_msg(void*);

char msg[BUFSZ], buf[BUFSZ];

int main(int argc, char* argv[]){
    int sock, group_id;
    int str_len, pos, mod;
    struct sockaddr_in serv_addr;
    pthread_t snd_thread, rcv_thread;
    void* thread_return;
    
    if(argc != 3){
        fprintf(stderr, "usage: %s <hostIP> <port>\n", argv[1]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    // connect server
    if(connect(sock, (struct sockaddr* )&serv_addr, sizeof(serv_addr)) == -1){
        error_handling("connect error");
    }

    // start chat
    pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
    pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);

    pthread_join(snd_thread, &thread_return);
    pthread_join(rcv_thread, &thread_return);

    return 0;
}

void error_handling(char* msg){
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

void receive_group_list(int sock, char* msg){
    int str_len, len = 0;
    while(1){
        str_len = read(sock, &msg[len], 1);
        if(str_len < 0){
            fprintf(stderr, "server connection lost\n");
            exit(1);
        }

        if(msg[len] == ESC){
            msg[len] = '\0';
            fprintf(stdout, "%s", msg);
            break;
        }

        if(msg[len] == '\n'){
            msg[len+1] = '\0';
            fprintf(stdout, "%s", msg);
            len = -1;
        }

        len++;
    }
    return;
}

int receive_msg(int sock, char* msg){
    int str_len, len = 0;

    while(1){
        str_len = read(sock, &msg[len], 1);
        if(str_len < 0){
            fprintf(stdout, "server connection lost\n");
            return -1;
        }

        if(msg[len] == ESC){
            msg[len] = '\0';
            break;
        }
        len++;
    }

    return len;
}

void* send_msg(void *arg) {
    int sock = *((int*)arg);
    char name_msg[BUFSZ];
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
    char name_msg[BUFSZ];
    int str_len;
    while(1) {
        str_len=read(sock, name_msg, BUFSZ);
        if(str_len == 0 || str_len == -1) 
            return (void*)-1;
        name_msg[str_len]='\0';
        fputs(name_msg, stdout);
        fflush(stdout);
    }
    return NULL;
}

