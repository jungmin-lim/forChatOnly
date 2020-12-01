#include <stdlib.h>
#include <curses.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "screen.h"
#include "bufstring.h"

#define MSG_WIDTH_SPACE 20
#define MSG_HEIGHT 5
#define MSG_SIZE 120
#define INPUT_SPACE 3

int lines, cols;
int mode;
char* msgptr = NULL;
int *ypos, *xpos;
int r_ypos, r_xpos;
WINDOW* remote_window = NULL;

void int_handler(int signo){
    WINDOW *popup = newwin(10, 40, (LINES - 10)/2, (COLS - 40)/2);
    int key;
    int answer = 0;
    box(popup, '|', '-');
    mvwprintw(popup, 4, 13, "%s", "Want to quit?");
    mvwprintw(popup, 6, 14, "%s", "Yes");
    wattrset(popup, A_STANDOUT);
    mvwprintw(popup, 6, 22, "%s", "No");
    wrefresh(popup);

    while(1){
        key = getch();
        switch(key){
            case '\n':
                if(answer == 1){
                    endwin();
                    exit(0);
                }
                else{
                    touchwin(stdscr);
                    refresh();
                    delwin(popup);
                    return;
                }
                break;
            case KEY_LEFT:
                if(answer == 0){
                    wattron(popup, A_STANDOUT);
                    mvwprintw(popup, 6, 14, "%s", "Yes");
                    wattroff(popup, A_STANDOUT);
                    mvwprintw(popup, 6, 22, "%s", "No");
                    answer = 1;
                }
                break;
            case KEY_RIGHT:
                if(answer == 1){
                    wattroff(popup, A_STANDOUT);
                    mvwprintw(popup, 6, 14, "%s", "Yes");
                    wattron(popup, A_STANDOUT);
                    mvwprintw(popup, 6, 22, "%s", "No");
                    answer = 0;
                }
                break;
            case 27:
                touchwin(stdscr);
                refresh();
                delwin(popup);
                return;
        }
        wmove(popup, 1, 0);
        wrefresh(popup);
    }
}

#ifdef DEBUG
int main(){
    int temp = 0;
    init();
    char n[100] = "myname";
    char msg[130] = "hello world";
    while(1){
        if(getstring(msg, 1) > 0){
            add_bubble(NULL, msg, temp);
        }
    }
    endwin();
    return 0;
}
#endif

void init(){
    signal(SIGINT, (void*)int_handler);
    initscr();
    scrollok(stdscr, TRUE);
    init_colorset();
    crmode();
    keypad(stdscr, TRUE);
    noecho();
    mode = 0;
    lines = LINES;
    cols = COLS;
    msgptr = NULL; ypos = NULL; xpos = NULL;
    clear();
}

void init_colorset(){
    start_color();
    init_pair(0, COLOR_WHITE, COLOR_BLACK);
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
}
char temp[1000] = "testtesttest";
int getstring(char* buf, int isChat){
    int i = 0, len = 0, fieldsize = COLS - 2*INPUT_SPACE;
    int key;
    int* curline;
    int* col;
    if(isChat){
        col = (int*)malloc(sizeof(int));
        curline = (int*)malloc(sizeof(int));
        *col = 0;
        *curline = LINES-MSG_HEIGHT;
        msgptr = buf;
        init_inputspace();
        move(LINES-MSG_HEIGHT, INPUT_SPACE);
    }
    else{
        fieldsize = COLS - 7;
        col = &r_xpos;
        curline = &r_ypos;
    }

    while((key = getch()) != '\n'){
        if(key >= ' ' && key < 127){
            if(isChat && len >= MSG_SIZE) continue;
    
            buf[len] = '\0';
            insert_char(buf, i++, key);
            (*col)++; len++;
            if(*col >= fieldsize){
                (*curline)++;
                *col = 0;
            }
            reset_inputfield(buf, *curline, *col);
        }
        else{
            switch(key){
                case KEY_BACKSPACE:
                    if(i > 0){
                        (*col)--;
                        buf[len] = '\0';
                        erase_char(buf, --i);
                        len--;
                        if(len != 0 && *col <= 0){
                            *col = fieldsize;
                            (*curline)--;
                        }
                        reset_inputfield(buf, *curline, *col);
                    }
                    break;

                case KEY_LEFT:
                    if(i > 0){
                        i--; (*col)--;
                        if(i != 0 && *col <= 0){
                            *col = fieldsize;
                            (*curline)--;
                        }
                        move(*curline, *col + INPUT_SPACE);
                    }
                    break;

                case KEY_RIGHT:
                    if(i < len){
                        i++; (*col)++;
                        if(*col >= fieldsize){
                            *col = 0;
                            (*curline)++;
                        }
                        move(*curline, *col + INPUT_SPACE);
                    }
                    break;

                case KEY_UP:
                    if(*curline > LINES - MSG_HEIGHT){
                        (*curline)--;
                        i -= fieldsize;
                        move(*curline, *col + INPUT_SPACE);
                    }
                    break;

                case KEY_DOWN:
                    if(i + fieldsize <= len){
                        (*curline)++;
                        i += fieldsize;
                        move(*curline, *col + INPUT_SPACE);
                    }
                    break;

                case KEY_DC:
                    if(i != len){
                        buf[len] = '\0';
                        erase_char(buf, i);
                        len--;
                        reset_inputfield(buf, *curline, *col);
                    }
                    break;

                case KEY_RESIZE:
                    if(lines != LINES) {
                        if(cols != COLS){
                            fieldsize = COLS - 2*INPUT_SPACE;
                            cols = COLS;
                        }
                        scrl(lines - LINES);
                        *curline -= lines - LINES;
                        move(*curline, i + INPUT_SPACE);
                        refresh();
                        lines = LINES;
                    }
                    break;
                case KEY_F(1):
                    init_remote();
                    getstring(temp, 0);
                    sleep(3);
                    end_remote();
                    break;
            }
        }
    }
    buf[len] = '\0';
    msgptr = NULL;
    if(isChat){
        free(col);
        free(curline);
    }
    return len;
}

void init_remote(){
    int width = COLS - 5, height = LINES - 3;
    remote_window = newwin(height, width, 3/2, 5/2);
    box(remote_window, '|', ' ');
    scrollok(remote_window, 1);
    mode = 1;
    r_ypos = 1; r_xpos = 1;
    wrefresh(remote_window);
}

void print_remote(char* msg){
    int width = COLS - 7, height = LINES - 5;
    int cur = width - r_ypos + 1;
    for(int i = 0; i < strlen(msg); i++){
        if(msg[i] != '\n'){
            mvwaddch(remote_window, r_ypos, r_xpos++, msg[i]);
        }
        else{
            r_ypos++;
            r_xpos = 1;
        }
        if(r_xpos >= width){
            r_xpos = 1;
            r_ypos++;
            if(r_ypos > height){
                r_ypos--;
                wscrl(remote_window, 1);
                box(remote_window, '|', ' ');
            }
        }
        wrefresh(remote_window);
    }
    wrefresh(remote_window);
}

void end_remote(){
    touchwin(stdscr);
    refresh();
    delwin(remote_window);
    remote_window = NULL;
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
    clear_inputspace();
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
    scrl(MSG_HEIGHT+1);
    if(msgptr != NULL){
        init_inputspace();
        reset_inputfield(msgptr, *ypos, *xpos);
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
    move(LINES-MSG_HEIGHT-1, INPUT_SPACE+1);
    printw("(  0/%3d)", MSG_SIZE);
}
