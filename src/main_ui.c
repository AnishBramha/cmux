#include "auth.h"

void show_login_screen(string_view* username, string_view* password, const char* errmsg) {

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);

    start_color();
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_WHITE, COLOR_RED);
    init_pair(3, COLOR_RED, COLOR_WHITE);

    int term_y, term_x;
    getmaxyx(stdscr, term_y, term_x);
    
    int win_height = 10;
    int win_width = 40;
    int start_y = (term_y - win_height) / 2;
    int start_x = (term_x - win_width) / 2;

    WINDOW* login_win = newwin(win_height, win_width, start_y, start_x);

    if (errmsg && errmsg[0]) {

        wbkgd(login_win, COLOR_PAIR(2));
        box(login_win, 0, 0);
        mvwprintw(login_win, 1, (win_width - 14) / 2, "cmux session");
        mvwprintw(login_win, 8, (win_width - (int)strlen(errmsg)) / 2, "%s", errmsg);
        wrefresh(login_win);
        napms(500);
    }

    wbkgd(login_win, COLOR_PAIR(1));
    box(login_win, 0, 0);

    mvwprintw(login_win, 1, (win_width - 14) / 2, "cmux session");

    if (errmsg && errmsg[0]) {

        wattron(login_win, COLOR_PAIR(3));
        mvwprintw(login_win, 8, (win_width - (int)strlen(errmsg)) / 2, "%s", errmsg);
        wattroff(login_win, COLOR_PAIR(3));
    }

    mvwprintw(login_win, 4, 4, "Username: ");
    mvwprintw(login_win, 6, 4, "Password: ");
    wrefresh(login_win);

    memset(username->str, 0, __USERNAME_LEN_MAX__);
    memset(password->str, 0, __PASSWORD_LEN_MAX__);

    echo(); 
    wmove(login_win, 4, 14);
    wgetnstr(login_win, username->str, __USERNAME_LEN_MAX__ - 1);
    username->len = strlen(username->str);

    noecho();
    wmove(login_win, 6, 14);
    wgetnstr(login_win, password->str, __PASSWORD_LEN_MAX__ - 1);
    password->len = strlen(password->str);

    delwin(login_win);
    endwin();
}




