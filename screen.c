#include <stdlib.h>
#include <curses.h>
#include <unistd.h>
#include <signal.h>

#define MSG_WIDTH_SPACE 20
#define MSG_HEIGHT 5
#define INPUT_SPACE 3

int lines, col;

void init();
void init_colorset();
void add_bubble(char* name, char* msg, int color);
int getstring(char* buf);
void clear_inputspace();
void init_inputspace();

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
		//scanw(" %s", msg);
		getstring(msg);
		clear_inputspace();
		add_bubble(n, msg, temp);
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
	col = COLS;
	clear();
}

void init_colorset(){
	start_color();
	init_pair(0, COLOR_WHITE, COLOR_BLACK);
	init_pair(1, COLOR_BLACK, COLOR_WHITE);
}

int getstring(char* buf){
	int i = 0, len = 0;
	int key;
	int curline = LINES-MSG_HEIGHT;

	while((key = getch()) != '\n'){
		if(key >= ' ' && key < 127){
			printw("%c", key);
			buf[i++] = key;
			len = len > i ? len : i;
		}
		else{
			switch(key){
				case KEY_LEFT:
					i--;
					move(curline, i + INPUT_SPACE);
					break;
				case KEY_RESIZE:
					if(lines != LINES) {
						scrl(lines - LINES);
						curline -= lines - LINES;
						move(curline, i + INPUT_SPACE);
						refresh();
						lines = LINES;
					}
					break;
			}
		}
	}
	buf[len] = '\0';
	return len;
}

void add_bubble(char* name, char* msg, int color){
	attrset(COLOR_PAIR(1));
	int space = name != NULL ? 2 : MSG_WIDTH_SPACE - 1;
	int where = LINES-MSG_HEIGHT-1;
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
		printw("%s", name);
	}
	move(where+2, space + 8);
	printw("%s", msg);
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
}
