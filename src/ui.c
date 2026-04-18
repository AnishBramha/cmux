#include "auth.h"
#include "common.h"
#include <fenv.h>
#include "ui.h"

#define CTRL(key) ((key) & 0x1F)

void show_login_screen(string_view* username, string_view* password, const char* errmsg) {

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
}



void show_dired(int server_fd, const char *username, pid_t pid) {

    DirRequest dreq = {.type = PKT_DIR_REQ};
    strncpy(dreq.username, username, __USERNAME_LEN_MAX__);

    if (send(server_fd, &dreq, sizeof dreq, 0) < 0) {
        panic("CLIENT [%d]: Failed to send directory request", pid);
    }

    DirResponse dres;
    if (recv(server_fd, &dres, sizeof dres, MSG_WAITALL) <= 0) {
        panic("CLIENT [%d]: Directory response dropped", pid);
    }

    mousemask(ALL_MOUSE_EVENTS, NULL);

    curs_set(0);

    int term_y, term_x;
    getmaxyx(stdscr, term_y, term_x);

    int dired_width = 30;
    WINDOW* win_dired = newwin(term_y, dired_width, 0, 0);
    WINDOW* win_editor = newwin(term_y, term_x - dired_width, 0, dired_width);

    box(win_dired, 0, 0);
    box(win_editor, 0, 0);
    mvwprintw(win_dired, 0, 2, "Dired");
    mvwprintw(win_editor, 0, 2, "Editor");

    int y = 2;

    forrange(size_t, i, 0, dres.nnodes, 1) {

        FileNode node = dres.nodes[i];
        wmove(win_dired, y++, 2);

        forrange(int, d, 0, node.depth, 1)
            wprintw(win_dired, "  ");

        wprintw(win_dired, ((node.type == NODE_DIR) ? "▼ %s" : "  %s"), node.name);
    }

    wrefresh(win_dired);
    wrefresh(win_editor);

    int c;
    while ((c = getch()) != CTRL('x')) {

        if (c == KEY_MOUSE) {

            MEVENT event;

            if (getmouse(&event) == OK) {

                if (event.x < dired_width) {

                    int clicked_idx = event.y - 2;

                    if (clicked_idx >= 0 && clicked_idx < dres.nnodes) {

                        if (dres.nodes[clicked_idx].type == NODE_FILE) {

                            FOpenRequest freq = {.type = PKT_FOPEN_REQ};
                            strncpy(freq.path, dres.nodes[clicked_idx].name, __FILENAME_LEN_MAX__);
                            send(server_fd, &freq, sizeof freq, 0);

                            FOpenResponse fres;
                            if (recv(server_fd, &fres, sizeof fres, MSG_WAITALL) <= 0)
                                break;

                            wclear(win_editor);
                            box(win_editor, 0, 0);
                            mvwprintw(win_editor, 0, 2, "Editor [%s]", freq.path);

                            if (fres.success) {

                                forrange(size_t, l, 0, fres.nlines, 1)
                                    mvwprintw(win_editor, l + 2, 2, "%4zu | %s", l + 1, fres.lines[l]);

                            } else
                                mvwprintw(win_editor, 2, 2, "Failed to open file");

                            wrefresh(win_editor);
                        }
                    }
                }
            }
        }
    }

    delwin(win_dired);
    delwin(win_editor);
}






