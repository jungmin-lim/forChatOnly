#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#include "screen.h"

#define BUFSZ 1024
#define NAMESZ 64
#define CLNTSZ 1024
#define ESC (char)27

void error_handling(char*);
void receive_group_list(int, char*);
int receive_user_list(int);
int receive_msg(int, char*);
void* send_msg(void*);
void* recv_msg(void*);

char msg[BUFSZ], buf[BUFSZ];
int user_count, user_id_list[CLNTSZ];
char user_name_list[CLNTSZ][NAMESZ];

int main(int argc, char* argv[]){
    int sock, group_id;
    int str_len, pos, mod;
    struct sockaddr_in serv_addr;
    pthread_t snd_thread, rcv_thread;
    void* thread_return;
    
    if(argc != 3){
        fprintf(stderr, "usage: %s <hostIP> <port>\n", argv[0]);
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

    while(1){
        // receive group list
        receive_group_list(sock, msg);

        while(1){
             // ask group to join
            fprintf(stdout, "\nSelect group: ");
            fflush(stdout);

            fgets(buf, sizeof(buf), stdin);
            buf[strlen(buf)-1] = '\0';

       
            // check if input is valid
            if((buf[0] == 'R' || buf[0] == 'r') && (strlen(buf) < 2)){
                mod = 0;
                break;
            }

            else if((buf[0] == 'N' || buf[0] == 'n') && (strlen(buf) < 2)){
                mod = 1;
                break;
            }
            else if(buf[0] >= '0' && buf[0] <= '9'){
                sscanf(buf, "%d", &group_id);
                if(0 <= group_id && group_id < 32){
                    mod = 2;
                    break;
                }
                else{
                    fprintf(stdout, "Invalid input try again\n");
                    continue;
                }
            }

            // invalid input
            else{
                fprintf(stdout, "Invalid input try again\n");
                continue;
            }
        }

        strcpy(msg, buf);
        write(sock, msg, sizeof(msg));

        msg[0] = ESC; msg[1] = '\0';
        write(sock, msg, strlen(msg));       
        
        // refresh
        if(mod == 0){
            continue;
        }

        // create new group
        else if(mod == 1){
            str_len = receive_msg(sock, msg);
            if(str_len < 0){
                close(sock);
                return 0;
            }

            // group creation failed
            if(msg[0] == '0'){
                fprintf(stdout, "%s", &msg[1]);
                continue;
            }
            
            // group creation success
            fprintf(stdout, "%s", &msg[1]);
            fflush(stdout);

            // ask group name
            while(1){
                fgets(buf, sizeof(buf), stdin);
                buf[strlen(buf)-1] = '\0';
                if(strlen(buf) > 63){
                    fprintf(stdout, "group name should shorter than 64 letters. try again: ");
                    fflush(stdout);
                }
                else{
                    strcpy(msg, buf);
                    write(sock, msg, strlen(msg));

                    msg[0] = ESC; msg[1] = '\0';
                    write(sock, msg, strlen(msg));
                    break;
                }
            }

            // ask user name
            str_len = receive_msg(sock, msg);
            if(str_len < 0){
                close(sock);
                return 0;
            }
            
            fprintf(stdout, "%s", msg);
            fflush(stdout);
            while(1){
                fgets(buf, sizeof(buf), stdin);
                buf[strlen(buf)-1] = '\0';
                if(strlen(buf) > 63){
                    fprintf(stdout, "user name should shorter than 64 letters. try again: ");
                    fflush(stdout);
                }
                else{
                    strcpy(msg, buf);
                    write(sock, msg, strlen(msg));

                    msg[0] = ESC; msg[1] = '\0';
                    write(sock, msg, strlen(msg));
                    break;
                }
            }

            // group creation complete
            str_len = receive_msg(sock, msg);
            if(str_len < 0){
                close(sock);
                return 0;
            }

            fprintf(stdout, "%s", msg);
            fflush(stdout);

            break;
        }

        // join existing group
        else if(mod == 2){
            str_len = receive_msg(sock, msg);
            if(str_len < 0 ){
                close(sock);
                return 0;
            }

            // group join failed
            if(msg[0] == '0'){
                fprintf(stdout, "%s", &msg[1]);
                continue;
            }

            // group join success
            fprintf(stdout, "%s", &msg[1]);
            fflush(stdout);

            // ask user name
            while(1){
                fgets(buf, sizeof(buf), stdin);
                buf[strlen(buf)-1] = '\0';

                if(strlen(buf) > 63){
                    fprintf(stdout, "user name should shorter than 64 letters. try again: ");
                    fflush(stdout);
                }
                else{
                    strcpy(msg, buf);
                    write(sock, msg, strlen(msg));

                    msg[0] = ESC; msg[1] = '\0';
                    write(sock, msg, strlen(msg));
                    break;
                }
            }

            // group join complete
            str_len = receive_msg(sock, msg);
            if(str_len < 0){
                close(sock);
                return 0;
            }

            fprintf(stdout, "%s", msg);
            fflush(stdout);
            break;
        }

    }

    init_screen();
    // start chat
    pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
    pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);

    pthread_join(snd_thread, &thread_return);
    pthread_join(rcv_thread, &thread_return);

    close(sock);
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
        if(str_len <= 0){
            fprintf(stderr, "server connection lost\n");
            exit(1);
        }

        if(msg[len] == ESC){
            msg[len] = '\0';
            fprintf(stdout, "%s", msg);
            fflush(stdout);
            break;
        }

        if(msg[len] == '\n'){
            msg[len+1] = '\0';
            fprintf(stdout, "%s", msg);
            fflush(stdout);
            len = -1;
        }

        len++;
    }
    return;
}

int receive_user_list(int sock){
    int str_len, len = 0;
    int user_count = 0, user_id;
    int buf[BUFSIZ], user_name[NAMESZ];

    sprintf(buf, "#init remote");
    write(sock, buf, strlen(buf));

    buf[0] = ESC; buf[1] = '\0';
    write(sock, buf, strlen(buf));

    while(1){
        str_len = read(sock, &buf[len], 1);
        if(str_len <= 0){
            fprintf(stderr, "server connection lost\n");
            exit(1);
        }

        if(buf[len] == ESC){

            break;
        }

        if(buf[len] == '\n'){
            buf[len] = '\0';
            sscanf(buf, "%d %s", &user_id, user_name);

            user_id_list[user_count] = user_id;
            strcpy(user_name_list[user_count], user_name);

            user_count++;
            len = -1;
        }

        len++;
    }

    return user_count;
}

int receive_msg(int sock, char* msg){
    int str_len, len = 0;

    while(1){
        str_len = read(sock, &msg[len], 1);
        if(str_len <= 0){
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
    char s_msg[BUFSZ];
    while(1) {
        s_msg[0] = '\0';
        getstring(s_msg, 1);
        // fgets(s_msg, sizeof(s_msg), stdin);
        // s_msg[strlen(s_msg)-1] = '\0';

        if(!strcmp(s_msg,"!exit")) {
            if(exit_handler() == 1){
                close(sock);
                exit(0);
            }
        }

        // remote
        else if(!strcmp(s_msg, "#init remote")){
            user_count = receive_user_list(sock);
            
        }

        // normal chat
        else{
            add_bubble(NULL, s_msg, 0);
            write(sock, s_msg, strlen(s_msg));

            s_msg[0] = ESC; s_msg[1] = '\0';
            write(sock, s_msg, strlen(s_msg));
        }

    }
    return NULL;
}

void* recv_msg(void *arg) {
    int sock = *((int*)arg);
    char r_msg[BUFSZ], msg[BUFSZ], name[BUFSZ];
    int str_len;
    while(1) {
        str_len = receive_msg(sock, r_msg);
        if(str_len < -1){
            close(sock);
            return (void*)-1;
        }

        sscanf(r_msg, "%s %s", name, msg);
        add_bubble(name, msg, 0);
        // fputs(r_msg, stdout);
        // fflush(stdout);
    }
    return NULL;
}

