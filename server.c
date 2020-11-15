#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <pthread.h>

#define NAMESZ 64
#define BUF_SIZE 1024
#define MAX_CLNT 512
#define MAX_GRP 32

typedef struct _node* nodePointer;
struct _node{
    int fd;
    char name[NAMESZ];
    int color;
    int state;
    nodePointer next;
}node;
typedef struct _group* groupPointer;
typedef struct _group{
    char name[NAMESZ];
    nodePointer list;
}group;

void * handle_clnt(void * arg);
void send_msg(int clnt_sock, char * msg, int len);
void error_handling(char * msg);
void initGroupList();
void initClntList();

nodePointer createNode(int fd);
int isEmpty(nodePointer list);
void insertCircularList(nodePointer list, nodePointer node);
void deleteCircularList(nodePointer node);

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

        // add client
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

/*
    new client connected
    1. send client list of existing group
    2. receive group selection from client
        2-1. if msg[0] == 'R', go back 1
        2-2. if msg[0] == 'N', create new group
        2-3. if connection fail, go back 1
    3. receive user name from client and join group
    4. receive client message and send it to other clients in group
*/
void * handle_clnt(void * arg){
    int clnt_sock=*((int*)arg);
    int str_len=0, group_id, i;
    char msg[BUF_SIZE]; 
    char buf[BUF_SIZE], com;

    // send group list to connected client
    while(!clntList[clnt_sock]->state){
        for(i = 0; i < MAX_GRP; ++i){
            if(groupList[i].list){
                sprintf(msg, "%d. %s\n", i, groupList[i].name);
                write(clnt_sock, msg, strlen(msg));
            }
        }

        strcpy(msg, "R. refresh\n");
        write(clnt_sock, msg, strlen(msg));
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
            sscanf(msg, "%c\n%s", &com, buf);
            for(i =0; i < MAX_GRP; ++i){
                if(!groupList[i].list){
                    pthread_mutex_lock(&mutx);
                    group_id = i;
                    strcpy(groupList[group_id].name, buf);
                    groupList[group_id].list = clntList[clnt_sock];
                    clntList[clnt_sock]->state = 1;
                    sprintf(buf, "OK: created group %d\n", group_id);
                    strcpy(msg, buf);
                    write(clnt_sock, msg, strlen(msg));
                    pthread_mutex_lock(&mutx);

                    // read user name from client
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

                    pthread_mutex_lock(&mutx);
                    strcpy(clntList[clnt_sock]->name, msg);
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
            sscanf(msg, "%d\n%s", &group_id, buf);
            if(!groupList[group_id].list){
                strcpy(msg, "FAIL: group not existing, try other gorup\n");
                write(clnt_sock, msg, strlen(msg));
            }
            else{
                pthread_mutex_lock(&mutx);
                insertCircularList(groupList[group_id].list, clntList[clnt_sock]);
                clntList[clnt_sock]->state = 1;
                sprintf(buf, "OK: joined group %s\n", groupList[group_id].name);
                strcpy(msg, buf);
                write(clnt_sock, msg, strlen(msg));
                pthread_mutex_unlock(&mutx);

                // read user name from client
                str_len = read(clnt_sock, msg, sizeof(msg));
                msg[str_len] = '\0';
                if(str_len <= 0){
                    pthread_mutex_lock(&mutx);
                    deleteCircularList(clntList[clnt_sock]);
                    clntList[clnt_sock] = NULL;
                    pthread_mutex_unlock(&mutx);

                    close(clnt_sock);
                    return NULL;
                }

                strcpy(clntList[clnt_sock]->name, msg);
            }
        }
    }

    /*
        read client message and send it to other clients in group
    */
    while((str_len=read(clnt_sock, msg, sizeof(msg)))!=0)
        send_msg(clnt_sock, msg, str_len);

    /*
        client eixt
        1. remove client from group
            1-1. if client is the only person in group, remove group
        2. remove client from client list
        3. close client socket
    */
    pthread_mutex_lock(&mutx);
    if(groupList[group_id].list == clntList[clnt_sock]){
        groupList[group_id].list = groupList[group_id].list->next;
        if(groupList[group_id].list == clntList[clnt_sock]){
            free(clntList[clnt_sock]);
            groupList[group_id].list = NULL;
            clntList[clnt_sock] = NULL;
        }
        else{
            deleteCircularList(clntList[clnt_sock]);
            clntList[clnt_sock] = NULL;
        }
    }
    pthread_mutex_unlock(&mutx);
    close(clnt_sock);
    return NULL;
}

void send_msg(int clnt_sock, char * msg, int len){
    nodePointer temp;
    temp = clntList[clnt_sock]->next;

    pthread_mutex_lock(&mutx);
    while(temp != clntList[clnt_sock]){
        write(temp->fd, msg, len);
        temp = temp->next;
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

nodePointer createNode(int fd){
    nodePointer newNode;
    time_t t;;

    srand((unsigned) time(&t));
    newNode = (nodePointer)malloc(sizeof(*newNode));
    if(newNode == NULL){
        fprintf(stderr, "error while allocating memory\n");
        exit(EXIT_FAILURE);
    }

    newNode->fd = fd;
    strcpy(newNode->name, "\0"); 
    newNode->color = rand()%10;
    newNode->state = 0;
    newNode->next = newNode;
    
    return newNode;
}

int isEmpty(nodePointer list){
    if( (!list) || (list->next == list)){
        return 1;
    }
    return 0;
}

void insertCircularList(nodePointer list, nodePointer node){
    node->next = list->next;
    list->next = node;

    return;
}

void deleteCircularList(nodePointer node){
    nodePointer temp;
    temp = node;

    if(isEmpty(node)) return;
    do{
        temp = temp->next;
    }while(temp != node);

    temp->next = node->next;
    free(node);

    return;
}
