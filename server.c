#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "user.h"

#define BUF_SIZE 1024
#define MAX_CLNT 512
#define MAX_GRP 32

typedef group* groupPointer;
typedef struct _group{
    char name[NAMESZ];
    nodePointer list;
}group;

void * handle_clnt(void * arg);         // function for multithread 
void send_msg(char * msg, int len);     // send message
void error_handling(char * msg);        // handling error
void initGroupList();
void initClntList();

int clnt_cnt=0;
nodePointer clntList[MAX_CLNT];
group groupList[MAX_GRP];
pthread_mutex_t mutx;

int main(int argc, char *argv[]){
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    int clnt_adr_sz;
    pthread_t t_id;
    if(argc!=2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    pthread_mutex_init(&mutx, NULL);
    serv_sock=socket(PF_INET, SOCK_STREAM, 0);
    initGroupList();
    initClntList();

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family=AF_INET; 
    serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_adr.sin_port=htons(atoi(argv[1]));

    if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1)
        error_handling("bind() error");
    if(listen(serv_sock, 5)==-1)
        error_handling("listen() error");

    while(1){
        // new client connected
        clnt_adr_sz=sizeof(clnt_adr);
        clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr,&clnt_adr_sz);

        // add clntList
        pthread_mutex_lock(&mutx);
        clntList[clnt_sock] = createNode(clnt_sock);
        pthread_mutex_unlock(&mutx);

        pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);
        pthread_detach(t_id);
        printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
    }

    close(serv_sock);
    return 0;
}

void * handle_clnt(void * arg){
    int clnt_sock=*((int*)arg);
    int str_len=0, i;
    char msg[BUF_SIZE]; 
    char buf[BUF_SIZE], buf1[BUF_SIZE];

    // send group list to connected clnt
    while(!clntList[clnt_sock]->state){
        for(i = 0; i < MAX_GRP; ++i){
            if(groupList[i].list){
                sprintf(msg, "%d. %s\n", i, groupList[i].name);
                write(clnt_sock, msg, strlen(msg));
            }
        }

        strcpy(msg, "R. refresh\n");
        write(clnt_sock, msg, srlen(msg));
        strcpy(msg, "N. create new group\n");
        write(clnt_sock, msg, strlen(msg));

        str_len = read(clnt_sock, msg, sizeof(msg));
        msg[str_len] = '\0';
        if(str_len <= 0){
            pthread_mutex_lock(&mutx);
            free(clntList[clnt_sock]);
            clntList[clnt_sock] = NULL;
            pthread_mutex_unlock(&mutx);

            close(clnt_sock);
            return NULL;
        }

        // create new group
        if(msg[0] == 'N' || msg[0] == 'n'){
            for(i =0; i < MAX_GRP; ++i){
                if(!groupList[i].list){
                    groupList[i].list = clntList[clnt_sock];
                    clntList[clnt_sock]->state = 1;
                    strcpy(msg, "OK: created group %d\n", i);
                    write(clnt_sock, msg, strlen(msg));

                    // read user name & group name 
                    str_len = read(clnt_sock, msg, sizeof(msg));
                    msg[str_len] = '\0';
                    if(str_len <= 0){
                        pthread_mutex_lock(&mutx);
                        free(clntList[clnt_sock]);
                        clntList[clnt_sock] = NULL;
                        pthread_mutex_unlock(&mutx);

                        close(clnt_sock);
                        return NULL;
                    }

                    sscanf(msg, "%s\n%s", buf, buf1);
                    pthread_mutex_lock(&mutx);
                    strcpy(clntList[clnt_sock]->name, buf);
                    strcpy(groupList[i].name, buf1);
                    pthread_mutex_unlock(&mutx);
                    break;
                }
            }

            // group full
            if(i == MAX_GRP){
                strcpy(msg, "FAIL: group FULL try later or join other group\n");
                write(clnt_sock, msg, strlen(msg));
            }
        }

        // refresh group list
        else if(msg[0] == 'R' || msg[0] == 'r'){
            continue;
        }

        // join existing group
        else{
            sscanf(msg, "%d\n%s", i, buf);
            if(!groupList[i].list){
                strcpy(msg, "FAIL: group not existing, try other gorup\n");
                write(clnt_sock, msg, strlen(msg));
            }
            else{
                pthread_mutex_lock(&mutx);
                insertCircularList(groupList[i].list, clntList[clnt_sock]);
                clntList[clnt_sock]->state = 1;
                pthread_mutex_unlock(&mutx);
                strcpy(msg, "OK: joined group %s\n", groupList[i].name);
                write(clnt_sock, msg, strlen(msg));
            }
        }
    }

    if(!clntList[clnt_sock]->state){
        pthread_mutex_lock(&mutx);
        free(clntList[clnt_sock]);
        clntList[clnt_sock] = NULL;
        pthread_mutex_unlock(&mutx);

        close(clnt_sock);
        return NULL;
    }

    while((str_len=read(clnt_sock, msg, sizeof(msg)))!=0)
        send_msg(msg, str_len);

    pthread_mutex_lock(&mutx);
    for(i=0; i<clnt_cnt; i++){
		if(clnt_sock==clnt_socks[i]){
            while(i++<clnt_cnt-1)
                clnt_socks[i]=clnt_socks[i+1];
            break;
        }
    }
    clnt_cnt--;
    pthread_mutex_unlock(&mutx);
    close(clnt_sock);
    return NULL;
}

void send_msg(char * msg, int len){
    nodePointer temp;
    pthread_mutex_lock(&mutx);
    for(i=0; i<clnt_cnt; i++){
        write(clnt_socks[i], msg, len);
    }
    pthread_mutex_unlock(&mutx);
}

void error_handling(char * msg){
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

void initGroupList(){
    int i;
    for(i = 0; i < MAX_GRP; ++i){
        groupList[i].list = NULL;
    }
}

void initClntList(){
    int i;
    for(i = 0; i < MAX_CLNT; ++i){
        clntList[i] = NULL;
    }
}