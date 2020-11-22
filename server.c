#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUFSZ   1024
#define NAMESZ  64
#define CLNTSZ  512
#define GRPSZ   32
#define ESC     (char) 27

typedef struct _client* clientPointer;
typedef struct _client {
    int fd;
    char name[NAMESZ];
    int color;
    int state;
    clientPointer next;
}client;

typedef struct _group* groupPointer;
typedef struct _group {
    char name[NAMESZ];
    clientPointer list;
}group;

void init_clnt_list ();
void init_grp_list ();
void* handle_clnt (void*);
void send_group_list (int clnt_sock);

// init global variables
pthread_mutex_t mutx;
clientPointer clnt_list[CLNTSZ];
groupPointer group_list[GRPSZ];

int main (int argc, char* argv[]) {
    struct sockaddr_in serv_adr, clnt_adr;
    int serv_sock, clnt_sock;
    int clnt_adr_sz;
    pthread_t t_id;

    // init
    pthread_mutex_init (&mutx, NULL);
    serv_sock = socket (PF_INET, SOCK_STREAM, 0);
    init_clnt_list ();
    init_grp_list ();

    memset(&serv_sock, 0, sizeof(serv_adr)); 
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr* )&serv_adr, sizeof(serv_adr)) == -1) {
        error_handling("bind() error");
    }

    if (listen(serv_sock, 5) == -1){
        error_handling("listen() error");
    }

    while (1) {
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr* )&clnt_adr, &clnt_adr_sz);

        pthread_mutex_lock (&mutx);
        clnt_list[clnt_sock] = create_node(clnt_sock);
        pthread_mutex_unlock(&mutx);

        pthread_create(&t_id, NULL, handle_clnt, (void* )&clnt_sock);
        pthread_detach(t_id);
        printf("Connected client IP: %s\n", inet_ntoa(clnt_adr.sin_addr));
    }

    close(serv_sock);
    return 0;
}

void error_handling(char * msg){
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

void init_clnt_list (){
    int i;
    for (i = 0; i < CLNTSZ; ++i) {
        clnt_list[i] = NULL;
    }

    return;
}

void int_grp_list (){
    int i;
    for(i = 0; i < GRPSZ; ++i) {
        group_list[i] = NULL;
    }

    return;
}

void* handle_clnt (void* arg){
    int clnt_sock = *((int* )arg);
    int str_len = 0, group_id, i;
    char msg[BUFSZ];

    while (!clnt_list[clnt_sock]->state) {
        send_group_list(clnt_sock);
    }
}

void send_group_list (int clnt_sock){
    int i;

    for(i = 0; i < GRPSZ; ++i){
        if(group_list[i]){

        }
    }
}