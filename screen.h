#ifndef _SCREEN_H_
#define _SCREEN_H_

int exit_handler();
void init();
void init_colorset();
void add_bubble(char* name, char* msg, int color);
int getstring(char* buf, int);
void clear_inputspace();
void init_inputspace();
void reset_inputfield(char*, int, int);
void init_remote();
void end_remote();
void print_remote(char* msg);
void reset_remotefield(char* msg, int *starty, int startx, int y, int x);

#endif