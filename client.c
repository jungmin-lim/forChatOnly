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

char msg[BUFSZ];
char buf[BUFSZ];

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

    // connect char gruop
    while(1){
        // receive group list
        receive_group_list(sock, msg);

        while(1){
             // ask group to join
            fprintf(stdout, "\nSelect group: ");
            fflush(stdout);

            scanf("%s\n", buf);
       
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
                sscanf(msg, "%d", &group_id);
                if(0 <= group_id && group_id < 32){
                    mod = 2;
                    break;
                }
            }

            // invalid input
            else{
                fprintf(stdout, "Invalid input try again\n");
            }
        }

        write(sock, msg, sizeof(msg));

        msg[0] = ESC; msg[1] = '\0';
        write(sock, msg, strlen(sock));       
        
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
                fprintf(stdout, &msg[1]);
                continue;
            }
            
            // group creation success
            fprintf(stdout, &msg[1]);
            fflush(stdout);

            // ask group name
            while(1){
                scanf("%s", buf);
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
            
            fprintf(stdout, msg);
            fflush(stdout);
            while(1){
                scanf("%s", buf);
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
                fprintf(stdout, &msg[1]);
                continue;
            }

            // group join success
            fprintf(stdout, &msg[1]);
            fflush(stdout);

            // ask user name
            while(1){
                scanf("%s", buf);

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
            break;
        }
    }
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
            fprintf(stdout, msg);
            break;
        }

        if(msg[len] == '\n'){
            msg[len+1] = '\0';
            fprintf(stdout, msg);
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