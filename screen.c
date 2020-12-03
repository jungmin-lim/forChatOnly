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

static int lines, cols;
static int mode;
static char* msgptr = NULL;
static int *ypos = NULL, *xpos = NULL;
static int r_ypos, r_xpos;
static int r_width, r_height;
static WINDOW* remote_window = NULL;
static WINDOW *popup = NULL;

void int_handler(int signo){
    if(exit_handler() == 1){
        endwin();
        exit(0);
    }
}

int exit_handler(){
    popup = newwin(10, 40, (LINES - 10)/2, (COLS - 40)/2);
    int key;
    int answer = 0, l = 1;
    box(popup, '|', '-');
    mvwprintw(popup, 4, 13, "%s", "Want to quit?");
    mvwprintw(popup, 6, 14, "%s", "Yes");
    wattrset(popup, A_STANDOUT);
    mvwprintw(popup, 6, 22, "%s", "No");
    wrefresh(popup);

    while(l){
        key = getch();
        switch(key){
            case '\n':
                l = 0;
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
                answer = 0;
                l = 0;
                break;
        }
        wmove(popup, 1, 0);
        wrefresh(popup);
    }
    touchwin(stdscr);
    refresh();
    delwin(popup);
    popup = NULL;
    return answer;
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

char temp[1000] = "testtesttesttesttesttesttesttesttest";

int getstring(char* buf, int isChat){
    int i = 0, len = 0, fieldsize = COLS - 2*INPUT_SPACE;
    int key;
    int starty, startx;
    int *ytemp, *xtemp;
    if(isChat){
        ytemp = ypos; xtemp = xpos;
        xpos = (int*)malloc(sizeof(int));
        ypos = (int*)malloc(sizeof(int));
        *xpos = 0;
        *ypos = LINES-MSG_HEIGHT;
        msgptr = buf;
        init_inputspace();
        move(LINES-MSG_HEIGHT, INPUT_SPACE);
    }
    else{
        fieldsize = r_width - 2;
        ytemp = ypos; xtemp = xpos;
        xpos = &r_xpos;
        ypos = &r_ypos;
        starty = r_ypos;
        startx = r_xpos;
    }

    while((key = getch()) != '\n'){
        if(key >= ' ' && key < 127){
            if(isChat && len >= MSG_SIZE) continue;
    
            buf[len] = '\0';
            insert_char(buf, i++, key);
            (*xpos)++; len++;
            if(*xpos >= fieldsize){
                (*ypos)++;
                *xpos = 0;
                if(!isChat) {
                    *xpos = 1;
                    if(*ypos > r_height - 2) (*ypos)--;
                }
            }
            if(isChat)reset_inputfield(buf, *ypos, *xpos);
            else reset_remotefield(buf, &starty, startx, *ypos, *xpos);
        }
        else{
            switch(key){
                case KEY_BACKSPACE:
                    if(i > 0){
                        (*xpos)--;
                        buf[len] = '\0';
                        erase_char(buf, --i);
                        len--;
                        if(isChat){
                            if(len != 0 && *xpos <= 0){
                                *xpos = fieldsize;
                                (*ypos)--;
                            }
                            reset_inputfield(buf, *ypos, *xpos);
                        }
                        else{
                            if(i != 0 && *xpos <= 1){
                                *xpos = fieldsize;
                                (*ypos)--;
                            }
                            reset_remotefield(buf, &starty, startx, *ypos, *xpos);
                            wrefresh(remote_window);
                        }
                    }
                    break;

                case KEY_LEFT:
                    if(i > 0){
                        i--; (*xpos)--;
                        if(isChat) {
                            if(i != 0 && *xpos <= 0){
                                *xpos = fieldsize;
                                (*ypos)--;
                            }
                            move(*ypos, *xpos + INPUT_SPACE);
                        }
                        else {
                            if(i != 0 && *xpos <= 1){
                                *xpos = fieldsize;
                                (*ypos)--;
                            }
                            wmove(remote_window, (*ypos), *xpos);
                            wrefresh(remote_window);
                        }
                    }
                    break;

                case KEY_RIGHT:
                    if(i < len){
                        i++; (*xpos)++;
                        if(isChat){
                            if(*xpos >= fieldsize){
                                *xpos = 0;
                                (*ypos)++;
                            }
                            move(*ypos, *xpos + INPUT_SPACE);
                        }
                        else{
                            if(*xpos >= fieldsize){
                                *xpos = 1;
                                (*ypos)++;
                            }
                            wmove(remote_window, *ypos, *xpos);
                            wrefresh(remote_window);
                        }
                    }
                    break;

                case KEY_UP:
                    if(isChat && *ypos > LINES - MSG_HEIGHT){
                        (*ypos)--;
                        i -= fieldsize;
                        move(*ypos, *xpos + INPUT_SPACE);
                    }
                    break;

                case KEY_DOWN:
                    if(isChat && i + fieldsize <= len){
                        (*ypos)++;
                        i += fieldsize;
                        move(*ypos, *xpos + INPUT_SPACE);
                    }
                    break;

                case KEY_DC:
                    if(i != len){
                        buf[len] = '\0';
                        erase_char(buf, i);
                        len--;
                        if(isChat)reset_inputfield(buf, *ypos, *xpos);
                        else{
                            reset_remotefield(buf, &starty, startx, *ypos, *xpos);
                            wrefresh(remote_window);
                        }
                    }
                    break;

                case KEY_RESIZE:
                    if(lines != LINES) {
                        if(cols != COLS){
                            fieldsize = COLS - 2*INPUT_SPACE;
                            cols = COLS;
                        }
                        scrl(lines - LINES);
                        *ypos -= lines - LINES;
                        move(*ypos, i + INPUT_SPACE);
                        refresh();
                        lines = LINES;
                    }
                    break;
                case KEY_F(1):
                    init_remote();
                    print_remote(temp);
                    getstring(temp, 0);
                    sleep(2);
                    end_remote();
                    add_bubble(temp, temp, 0);
                    break;
            }
        }
    }
    buf[len] = '\0';
    if(isChat){
        msgptr = NULL;
        free(xpos);
        free(ypos);
    }
    xpos = xtemp; ypos = ytemp;
    return len;
}

void init_remote(){
    r_width = COLS - 5, r_height = LINES - 3;
    remote_window = newwin(r_height, r_width, 3/2, 5/2);
    box(remote_window, '|', ' ');
    scrollok(remote_window, 1);
    mode = 1;
    r_ypos = 1; r_xpos = 1;
    wrefresh(remote_window);
}

void print_remote(char* msg){
    int width = r_width - 2, height = r_height - 2;
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

void reset_remotefield(char* msg, int* starty, int startx, int y, int x){
    int fieldsize = r_width - 2;
    int i = 1;

    mvwprintw(remote_window, *starty, startx, "%*s", fieldsize - startx, "");
    for(int i = (*starty)+1; i < r_height - 2; i++){
        mvwprintw(remote_window, i, 1, "%*s", fieldsize - 1, "");
    }
    mvwprintw(remote_window, *starty, startx, "%.*s", fieldsize - startx, msg);
    for(int cur = fieldsize - startx; cur < strlen(msg); cur+=fieldsize - 1){
        mvwprintw(remote_window, (*starty) + i, 1, "%.*s", fieldsize - 1, msg + cur);
        i++;
        if((*starty) + i > r_height - 1){
            (*starty)--;
            wscrl(remote_window, 1);
            box(remote_window, '|', ' ');
        }
    }
    wmove(remote_window, y, x);
    wrefresh(remote_window);
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
    msg[0] = '\0';
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
