#include <stdlib.h>
#include <curses.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include "screen.h"
#include "bufstring.h"

#define MSG_WIDTH_SPACE 20
#define MSG_HEIGHT 5
#define MSG_SIZE 120
#define INPUT_SPACE 3

static int lines, cols;
static char* msgptr = NULL;
static int *ypos = NULL, *xpos = NULL;
static int r_ypos, r_xpos;
static int r_width, r_height;
static int isChatting;
static int recv_remote_data;
static WINDOW* remote_window = NULL;
static WINDOW *popup = NULL;
static pthread_mutex_t mutx;

void init_colorset();
void clear_inputspace();
void init_inputspace();
void reset_inputfield(char*, int, int);
void reset_remotefield(char* msg, int *starty, int startx, int y, int x);
int dialog_yes_or_no(char* msg);

void int_handler(int signo){
    endwin();
    exit(0);
}

int getOneByte(int isChat){
    pthread_mutex_lock(&mutx);
    int ret = getch();
    if(isChat != isChatting) {
        ungetch(ret);
        ret = 0;
    }
    pthread_mutex_unlock(&mutx);
    return ret;
}

int dialog_yes_or_no(char* msg){
    popup = newwin(10, 40, (LINES - 10)/2, (COLS - 40)/2);
    keypad(popup, TRUE);
    int key;
    int answer = 0, l = 1;
    int temp = isChatting;
    isChatting = 2;
    box(popup, '|', '-');
    mvwprintw(popup, 4, (40 - strlen(msg))/2, "%s", msg);
    mvwprintw(popup, 6, 14, "%s", "Yes");
    wattrset(popup, A_STANDOUT);
    mvwprintw(popup, 6, 22, "%s", "No");
    wrefresh(popup);

    while(l){
        key = getOneByte(2);
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
    isChatting = temp;
    if(answer == 1){
        endwin();
    }
    return answer;
}

void dialog_msg(char* msg){
    popup = newwin(10, 50, (LINES - 10)/2, (COLS - 50)/2);
    box(popup, '|', '-');

    mvwprintw(popup, 5, (50 - strlen(msg))/2, "%s", msg);
    wrefresh(popup);
    sleep(1);

    touchwin(stdscr);
    refresh();
    delwin(popup);
    popup = NULL;
}

int exit_handler(){
    return dialog_yes_or_no("Want to quit?");
}

int remote_request(char* msg){
    char buf[BUFSIZ];
    sprintf(buf, "remote access requested from %s", msg);
    
    return dialog_yes_or_no(buf);
}

#ifdef DEBUG
int main(){
    int temp = 0;
    init_screen();
    char msg[130] = "hello world";
    int id[3] = {3, 4, 5};
    char name[3][64] = {"hello", "my", "name is screen.c"};
    while(1){
        if(getstring(msg, 1) > 0){
            add_bubble(NULL, msg, temp);
        }
        temp = choose_from_array(3, id, name);
        sprintf(msg, "%d", temp);
        add_bubble(NULL, msg, 0);
    }
    endwin();
    return 0;
}
#endif

void init_screen(){
    signal(SIGINT, (void*)int_handler);
    signal(SIGQUIT, (void*)int_handler);
    initscr();
    scrollok(stdscr, TRUE);
    init_colorset();
    crmode();
    keypad(stdscr, TRUE);
    noecho();
    lines = LINES;
    cols = COLS;
    isChatting = 1;
    msgptr = NULL; ypos = NULL; xpos = NULL;
    pthread_mutex_init(&mutx, NULL);
    clear();
}

void init_colorset(){
    start_color();
    init_pair(0, COLOR_WHITE, COLOR_BLACK);
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
}

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

    while(1){
        key = getOneByte(isChat);
        if(isChatting == 0 && isChat == 1) {
            len = 0;
            break;
        }
        if(key == '\n') break;
        if(!isChat && recv_remote_data){
            starty = r_ypos;
            startx = r_xpos;
        }
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
            else {
                reset_remotefield(buf, &starty, startx, *ypos, *xpos);
            }
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
                    sleep(2);
                    end_remote();
                    break;
            }
        }
        recv_remote_data = 0;
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
    isChatting = 0;
    r_ypos = 1; r_xpos = 1;
    recv_remote_data = 0;
    wrefresh(remote_window);
}

void print_remote(char* msg){
    int width = r_width - 2, height = r_height - 2;
    int cur = width - r_ypos + 1;
    recv_remote_data = 1;
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
    isChatting = 1;
    remote_window = NULL;
}

int choose_from_array(int count, int* id, char name[][NAMESZ]){
    WINDOW* list_popup = newwin(count+2, NAMESZ + 8, (LINES - count + 2)/2, (COLS - NAMESZ + 8)/2);
    int c = -1;
    int ans = 0, loop = 1;
    int temp = isChatting;
    keypad(list_popup, true);
    isChatting = 3;
    box(list_popup, '|', '-');
    do{
        switch(c){
            case KEY_UP:
                if(ans > 0) ans--;
                break;
            case KEY_DOWN:
                if(ans < count - 1) ans++;
                break;
            case '\n':
                loop = 0;
                break;
        }
        for(int i = 0; loop && i < count; i++){
            if(i == ans)
                wattron(list_popup, A_STANDOUT);

            mvwprintw(list_popup, i + 1, 2, "%d: %-*s", id[i], NAMESZ - 2, name[i]);

            if(i == ans)
                wattroff(list_popup, A_STANDOUT);
        }
        wrefresh(list_popup);
    } while(loop && (c = getOneByte(3)));
    touchwin(stdscr);
    refresh();
    delwin(list_popup);
    isChatting = temp;
    return id[ans];
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
    for(int i = (*starty)+1; i < r_height - 1; i++){
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
        printw("%s", name);
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
