#ifndef _SCREEN_H_
#define _SCREEN_H_

#define NAMESZ 64

int exit_handler();
void init_screen();
void add_bubble(char* name, char* msg, int color);
int getstring(char* buf, int isChat);
void init_remote();
void end_remote();
void print_remote(char* msg);
int choose_from_array(int count, int* id, char name[][NAMESZ]);

#endif