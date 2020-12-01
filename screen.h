#ifndef _SCREEN_H_
#define _SCREEN_H_

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

#endif