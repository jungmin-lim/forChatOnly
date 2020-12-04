#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUFSZ 1024
#define NAMESZ 64
#define CLNTSZ 1024
#define GRPSZ 32
#define ESC (char)27

typedef struct _client *clientPointer;
typedef struct _client
{
    int fd;
    char name[NAMESZ];
    int color;
    int state;
    clientPointer next;
} client;

typedef struct _group *groupPointer;
typedef struct _group
{
    char name[NAMESZ];
    clientPointer list;
} group;

void error_handling(char*);
void init_clnt_list();
void init_grp_list();
clientPointer create_clnt(int);
groupPointer create_group(clientPointer);
void remove_clnt(int);
void remove_group(int);
void join_group(int, int);
void exit_group(int, int);
int receive_msg(int, char*);
void send_group_list(int);
void send_user_list(int);
void *handle_clnt(void *);

// init global variables
pthread_mutex_t mutx;
clientPointer clnt_list[CLNTSZ];
groupPointer group_list[GRPSZ];

int main(int argc, char *argv[]){
    struct sockaddr_in serv_adr, clnt_adr;
    int serv_sock, clnt_sock;
    int clnt_adr_sz;
    pthread_t t_id;

    if(argc!=2){
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    // init
    pthread_mutex_init(&mutx, NULL);
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    init_clnt_list();
    init_grp_list();

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1){
        error_handling("bind() error");
    }

    if (listen(serv_sock, 5) == -1){
        error_handling("listen() error");
    }

    while (1){
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
        // TODO: if client list full
        if (clnt_sock >= CLNTSZ){
        }

        pthread_mutex_lock(&mutx);
        clnt_list[clnt_sock] = create_clnt(clnt_sock);
        pthread_mutex_unlock(&mutx);

        pthread_create(&t_id, NULL, handle_clnt, (void *)&clnt_sock);
        pthread_detach(t_id);
        printf("Connected client IP: %s\n", inet_ntoa(clnt_adr.sin_addr));
    }

    close(serv_sock);
    return 0;
}

void error_handling(char *msg){
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

void init_clnt_list(){
    int i;
    for (i = 0; i < CLNTSZ; ++i){
        clnt_list[i] = NULL;
    }

    return;
}

void init_grp_list(){
    int i;
    for (i = 0; i < GRPSZ; ++i){
        group_list[i] = NULL;
    }

    return;
}

clientPointer create_clnt(int clnt_sock){
    clientPointer new_clnt;
    time_t t;

    srand((unsigned)time(&t));
    new_clnt = (clientPointer)malloc(sizeof(*new_clnt));
    if (new_clnt == NULL){
        fprintf(stderr, "error allocating memory\n");
        exit(1);
    }

    new_clnt->fd = clnt_sock;
    new_clnt->state = 0;
    new_clnt->color = rand() % 10;
    new_clnt->next = new_clnt;
    strcpy(new_clnt->name, "\0");

    return new_clnt;
}

groupPointer create_group(clientPointer creator){
    groupPointer new_group;

    new_group = (groupPointer)malloc(sizeof(*new_group));
    if (new_group == NULL){
        fprintf(stderr, "error allocating memory\n");
        exit(EXIT_FAILURE);
    }

    strcpy(new_group->name, "\0");
    new_group->list = creator;
    return new_group;
}

void remove_clnt(int clnt_sock){
    free(clnt_list[clnt_sock]);
    clnt_list[clnt_sock] = NULL;

    return;
}

void remove_group(int group_id){
    free(group_list[group_id]);
    group_list[group_id] = NULL;

    return;
}

void join_group(int group_id, int clnt_sock){
    clientPointer temp;
    temp = group_list[group_id]->list->next;

    clnt_list[clnt_sock]->next = temp;
    group_list[group_id]->list->next = clnt_list[clnt_sock];
    return;
}

void exit_group(int group_id, int clnt_sock){
    clientPointer temp = group_list[group_id]->list;

    while (temp->next != clnt_list[clnt_sock]){
        temp = temp->next;
    }

    // if clnt is first user of the group
    if (clnt_list[clnt_sock] == group_list[group_id]->list){
        // if clnt is the only user in the group
        if (clnt_list[clnt_sock]->next == clnt_list[clnt_sock]){
            // delete group
            remove_group(group_id);
        }
        else{
            // exit group
            temp->next = clnt_list[clnt_sock]->next;
            group_list[group_id]->list = clnt_list[clnt_sock]->next;
        }
    }
    else{
        temp->next = clnt_list[clnt_sock]->next;
    }

    // set clnt initial state
    clnt_list[clnt_sock]->next = clnt_list[clnt_sock];
    clnt_list[clnt_sock]->state = 0;
    strcpy(clnt_list[clnt_sock]->name, "\0");

    return;
}

int receive_msg(int clnt_sock, char* msg){
    int str_len, len = 0;
    msg[0] = '\0';

    while(1){
        str_len = read(clnt_sock, &msg[len], 1);
        if(str_len <= 0){
            fprintf(stdout, "client %d connection lost\n", clnt_sock);
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

void send_group_list(int clnt_sock){
    char msg[BUFSZ];
    int i;

    // traverse group list and send number & name of existing group
    for (i = 0; i < GRPSZ; ++i){
        if (group_list[i]){
            sprintf(msg, "%d. %s\n", i, group_list[i]->name);
            write(clnt_sock, msg, strlen(msg));
        }
    }

    strcpy(msg, "R. refresh group list\n");
    write(clnt_sock, msg, strlen(msg));

    strcpy(msg, "N. create new group\n");
    write(clnt_sock, msg, strlen(msg));

    // write ESC to indicate end of list
    msg[0] = ESC; msg[1] = '\0';
    write(clnt_sock, msg, strlen(msg));

    return;
}

void send_user_list(int clnt_sock){
    char msg[BUFSZ];
    clientPointer temp = clnt_list[clnt_sock];

    temp = temp->next;
    while(temp != clnt_list[clnt_sock]){
        sprintf(msg, "%d %s\n", temp->fd, temp->name);
        write(clnt_sock, msg, strlen(msg));
    }

    msg[0] = ESC; msg[1] = '\0';
    write(clnt_sock, msg, strlen(msg));

    return;
}

void *handle_clnt(void *arg){
    clientPointer temp;
    int clnt_sock = *((int *)arg);
    int str_len = 0, group_id, i;
    char msg[BUFSZ], buf[BUFSZ];

    while (1){
        /*
            new client connected
                1. send client list of groups
                2. receive group selection from client
        */
        while (clnt_list[clnt_sock]->state == 0){
            send_group_list(clnt_sock);
            fprintf(stdout, "group list sent\n");

            str_len = receive_msg(clnt_sock, msg);
            fprintf(stdout, "%s\n", msg);
            if (str_len < 0){
                remove_clnt(clnt_sock);
                close(clnt_sock);
                return NULL;
            }

            // refresh group list
            if (msg[0] == 'R' || msg[0] == 'r'){
                continue;
            }

            // create new group
            else if (msg[0] == 'N' || msg[0] == 'n'){
                for (group_id = 0; group_id < GRPSZ; ++group_id){
                    if (group_list[group_id] == NULL)
                        break;
                }

                // group list full cannot create new group
                if (group_id == GRPSZ){
                    // send group full message
                    pthread_mutex_lock(&mutx);
                    sprintf(msg, "0group list is full. Try later.\n");
                    write(clnt_sock, msg, strlen(msg));

                    msg[0] = ESC;
                    msg[1] = '\0';
                    write(clnt_sock, msg, strlen(msg));
                    pthread_mutex_unlock(&mutx);
                }
                else{
                    pthread_mutex_lock(&mutx);
                    group_list[group_id] = create_group(clnt_list[clnt_sock]);
                    pthread_mutex_unlock(&mutx);

                    // ask group name
                    sprintf(msg, "1input group name to create: ");
                    write(clnt_sock, msg, strlen(msg));

                    msg[0] = ESC;
                    msg[1] = '\0';
                    write(clnt_sock, msg, strlen(msg));

                    str_len = receive_msg(clnt_sock, msg);
                    if (str_len < 0){
                        remove_group(group_id);
                        remove_clnt(clnt_sock);
                        close(clnt_sock);
                        return NULL;
                    }

                    // set group name
                    strcpy(group_list[group_id]->name, msg);
                    fprintf(stdout, "set group name\n");

                    // ask user name
                    sprintf(msg, "input user name to use: ");
                    write(clnt_sock, msg, strlen(msg));

                    msg[0] = ESC;
                    msg[1] = '\0';
                    write(clnt_sock, msg, strlen(msg));

                    str_len = receive_msg(clnt_sock, msg);
                    if (str_len < 0){
                        remove_group(group_id);
                        remove_clnt(clnt_sock);
                        close(clnt_sock);
                        return NULL;
                    }

                    // set user name
                    strcpy(clnt_list[clnt_sock]->name, msg);
                    fprintf(stdout, "set user name\n");

                    // send group joined message
                    sprintf(msg, "joined %s\n", group_list[group_id]->name);
                    write(clnt_sock, msg, strlen(msg));

                    msg[0] = ESC; msg[1] = '\0';
                    write(clnt_sock, msg, strlen(msg));

                    // group creation complete. set state 1
                    clnt_list[clnt_sock]->state = 1;
                    fprintf(stdout, "group joined\n");
                }
            }

            // join group
            else {
                sscanf(msg, "%d", &group_id);
                fprintf(stdout, "groupid: %d\n", group_id);
                // group not exist
                if (group_list[group_id] == NULL) {
                    sprintf(msg, "0group id %d does not exists. Choose other group\n", group_id);
                    write(clnt_sock, msg, strlen(msg));

                    msg[0] = ESC;
                    msg[1] = '\0';
                    write(clnt_sock, msg, strlen(msg));

                    continue;
                }

                else{
                    join_group(group_id, clnt_sock);

                    // ask user name
                    sprintf(msg, "1input user name to use: ");
                    write(clnt_sock, msg, strlen(msg));

                    msg[0] = ESC;
                    msg[1] = '\0';
                    write(clnt_sock, msg, strlen(msg));

                    str_len = receive_msg(clnt_sock, msg);
                    if (str_len < 0){
                        exit_group(group_id, clnt_sock);
                        remove_clnt(clnt_sock);
                        close(clnt_sock);
                        return NULL;
                    }

                    // set user name
                    strcpy(clnt_list[clnt_sock]->name, msg);

                    fprintf(stdout, "set user name\n");

                    // send group joined message
                    sprintf(msg, "joined %s\n", group_list[group_id]->name);
                    write(clnt_sock, msg, strlen(msg));

                    msg[0] = ESC; msg[1] = '\0';
                    write(clnt_sock, msg, strlen(msg));

                    // group join complete. set state 1
                    clnt_list[clnt_sock]->state = 1;
                    fprintf(stdout, "group joined\n");
                }

            }
        }

        fprintf(stdout, "receiving messages\n");
        // receive message and broadcast
        while(clnt_list[clnt_sock]->state == 1){
            str_len = receive_msg(clnt_sock, msg);
            if(str_len < 0){
                exit_group(group_id, clnt_sock);
                remove_clnt(clnt_sock);
                close(clnt_sock);
                return NULL;
            }

            // add user name on message 
            strcpy(buf, msg);
            sprintf(msg, "[%s] %s", clnt_list[clnt_sock]->name, buf);

            temp = clnt_list[clnt_sock]->next;
            buf[0] = ESC; buf[1] = '\0';

            // broadcast message for every user in group
            while(temp != clnt_list[clnt_sock]){
                write(temp->fd, msg, strlen(msg));
                write(temp->fd, buf, strlen(buf));
                temp = temp->next;
            }
        }
    }
}
