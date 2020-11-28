#include <stdlib.h>
#include <curses.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "bufstring.h"

#define MSG_WIDTH_SPACE 20
#define MSG_HEIGHT 5
#define MSG_SIZE 120
#define INPUT_SPACE 3

int lines, cols;

void init();
void init_colorset();
void add_bubble(char* name, char* msg, int color);
int getstring(char* buf);
void clear_inputspace();
void init_inputspace();
void reset_inputfield(char*, int, int);

void int_handler(int signo){
	endwin();
	exit(0);
}

int main(){
	int temp = 0;
	init();
	attrset(COLOR_PAIR(1));
	char n[100] = "myname";
	char msg[100] = "hello world";
	while(1){
		init_inputspace();
		move(LINES-MSG_HEIGHT, INPUT_SPACE);
		getstring(msg);
		clear_inputspace();
		add_bubble(NULL, msg, temp);
		temp = temp == 1?0:1;
		scrl(MSG_HEIGHT+1);
	}
	endwin();
	return 0;
}

void init(){
	signal(SIGINT, (void*)int_handler);
	initscr();
	scrollok(stdscr, TRUE);
	init_colorset();
	crmode();
	keypad(stdscr, TRUE);
	noecho();
	lines = LINES;
	cols = COLS;
	clear();
}

void init_colorset(){
	start_color();
	init_pair(0, COLOR_WHITE, COLOR_BLACK);
	init_pair(1, COLOR_BLACK, COLOR_WHITE);
}

int getstring(char* buf){
	int i = 0, col = 0, len = 0, fieldsize = COLS - 2*INPUT_SPACE;
	int key;
	int curline = LINES-MSG_HEIGHT;

	while((key = getch()) != '\n'){
		if(key >= ' ' && key < 127){
			if(len >= MSG_SIZE) continue;
	
			buf[len] = '\0';
			insert_char(buf, i++, key);
			col++; len++;
			if(col >= fieldsize){
				curline++;
				col = 0;
			}
			reset_inputfield(buf, curline, col);
			
		}
		else{
			switch(key){
				case KEY_BACKSPACE:
					if(i > 0){
						col--;
						buf[len] = '\0';
						erase_char(buf, --i);
						len--;
						if(len != 0 && col <= 0){
							col = fieldsize;
							curline--;
						}
						reset_inputfield(buf, curline, col);
					}
					break;

				case KEY_LEFT:
					if(i > 0){
						i--; col--;
						if(col <= 0){
							col = fieldsize;
							curline--;
						}
						move(curline, col + INPUT_SPACE);
					}
					break;

				case KEY_RIGHT:
					if(i < len){
						i++; col++;
						if(col >= fieldsize){
							col = 0;
							curline++;
						}
						move(curline, col + INPUT_SPACE);
					}
					break;

				case KEY_UP:
					if(curline > LINES - MSG_HEIGHT){
						curline--;
						i -= fieldsize;
						move(curline, col + INPUT_SPACE);
					}
					break;

				case KEY_DOWN:
					if(i + fieldsize <= len){
						curline++;
						i += fieldsize;
						move(curline, col + INPUT_SPACE);
					}
					break;

				case KEY_DC:
					if(i != len){
						buf[len] = '\0';
						erase_char(buf, i);
						len--;
						reset_inputfield(buf, curline, col);
					}
					break;

				case KEY_RESIZE:
					if(lines != LINES) {
						scrl(lines - LINES);
						curline -= lines - LINES;
						move(curline, i + INPUT_SPACE);
						refresh();
						lines = LINES;
						cols = COLS;
					}
					break;
			}
		}
	}
	buf[len] = '\0';
	return len;
}

void reset_inputfield(char* msg, int y, int x){
	int fieldsize = COLS - 2*INPUT_SPACE;
	int i = 0;
	clear_inputspace();
	init_inputspace();
	move(LINES-6, INPUT_SPACE+2);
	printw("%3d", strlen(msg));
	move(LINES-MSG_HEIGHT, INPUT_SPACE);
	for (int cur = 0; cur < strlen(msg); cur += fieldsize){
		move(LINES-MSG_HEIGHT+i, INPUT_SPACE);
		printw("%.*s", fieldsize, msg + cur);
		i++;
	}
	move(y, INPUT_SPACE+x);
}

void add_bubble(char* name, char* msg, int color){
	attrset(COLOR_PAIR(1));
	int space = name != NULL ? 2 : MSG_WIDTH_SPACE - 1;
	int where = LINES-MSG_HEIGHT-1;
	int i = 0, bubblewidth = COLS - MSG_WIDTH_SPACE - 12;
	move(where, space + 2);
	printw("%*s",  COLS - MSG_WIDTH_SPACE - 4,"");
	move(where+1, space + 1);
	printw("%*s", COLS - MSG_WIDTH_SPACE - 2, "");
	move(where+2, space);
	printw("%*s", COLS - MSG_WIDTH_SPACE, "");
	move(where+3, space + 1);
	printw("%*s", COLS - MSG_WIDTH_SPACE - 2, "");
	move(where+4, space + 2);
	printw("%*s", COLS - MSG_WIDTH_SPACE - 4, "");

	if(name != NULL) {
		move(where+1, space + 8);
		printw("[%s]", name);
	}
	for (int cur = 0; cur < strlen(msg); cur += bubblewidth){
		move(where+2+i, space + 8);
		printw("%.*s", bubblewidth, msg + cur);
		i++;
	}
	refresh();
}	

void clear_inputspace(){
	attrset(COLOR_PAIR(0));
	for(int i = 0; i <= MSG_HEIGHT; i++){
		move(LINES-i-2, 0);
		printw("%*s", COLS, "");
	}
}

void init_inputspace(){
	attrset(COLOR_PAIR(1));
	for(int i = 0; i < MSG_HEIGHT; i++){
		move(LINES-i-2, INPUT_SPACE-2);
		printw("%*s", COLS - 2*(INPUT_SPACE-2), "");
	}
	move(LINES-6, INPUT_SPACE+1);
	printw("(  0/%3d)", MSG_SIZE);
}
