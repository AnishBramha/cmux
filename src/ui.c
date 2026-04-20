#include "common.h"
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



void show_dired(int server_fd, const char* username, pid_t pid) {

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
    mvwprintw(win_editor, 0, term_x - dired_width - 12, "[< Back ]");

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

    bool in_editor = false;
    char current_file[__FILENAME_LEN_MAX__] = {NIL};
    int cursor_y = 0;
    int cursor_x = 0;
    int line_offt = 7;

    pid_t locked_lines[__FILE_LINES_MAX__] = {0};
    wtimeout(stdscr, 50);

    char mk_buf[__FILENAME_LEN_MAX__] = {NIL};
    int mk_len = 0;
    char sh_buf[__USERNAME_LEN_MAX__] = {NIL};
    int sh_len = 0;
    int dired_focus = 0;
    char msg[__ERRMSG_LEN_MAX__] = {NIL};
    
    int c;
    while ((c = getch()) != CTRL('x')) {

        if (c == ERR) {

            if (!in_editor) {

                DirRequest req = {.type = PKT_DIR_REQ};
                strncpy(req.username, username, __USERNAME_LEN_MAX__);
                send(server_fd, &req, sizeof req, 0);

                if (recv(server_fd, &dres, sizeof dres, MSG_WAITALL) > 0) {

                    wclear(win_dired);
                    box(win_dired, 0, 0);
                    mvwprintw(win_dired, 0, 2, "Dired");

                    int file_y = 2;
                    forrange(size_t, i, 0, dres.nnodes, 1) {

                        FileNode node = dres.nodes[i];
                        wmove(win_dired, file_y++, 2);

                        forrange(int, d, 0, node.depth, 1)
                            wprintw(win_dired, " ");
                        wprintw(win_dired, (node.type == NODE_DIR ? "v %s" : "  %s"), dres.nodes[i].name);
                    }
                }
            }

            int dy = term_y - 6;
            mvwprintw(win_dired, dy, 2, "Filename:");
            mvwprintw(win_dired, dy + 1, 2, "[ %-20s ]", mk_buf);
            mvwprintw(win_dired, dy + 2, 2, "Share with:");
            mvwprintw(win_dired, dy + 3, 2, "[ %-20s ]", sh_buf);

            if (msg[0])
                mvwprintw(win_dired, dy + 4, 2, "%-26s", msg);

            wrefresh(win_dired);

            if (in_editor) {

                SyncRequest sreq = {
                    
                    .type = PKT_SYNC_REQ,
                    .cursor_line = cursor_y,
                    .clipid = pid,
                };
                strncpy(sreq.path, current_file, __FILENAME_LEN_MAX__ - 1);
                send(server_fd, &sreq, sizeof sreq, 0);

                SyncResponse sres;
                if (recv(server_fd, &sres, sizeof sres, MSG_WAITALL) > 0) {

                    if (sres.success) {

                        forrange(size_t, l, 0, sres.nlines, 1) {

                            locked_lines[l] = sres.lines[l].locked;
                            
                            if (sres.lines[l].locked != 0 && sres.lines[l].locked != pid) {

                                wattron(win_editor, COLOR_PAIR(2));
                                mvwprintw(win_editor, l + 2, 2, "%2zu ", l + 1, sres.lines[l].text);
                                wattroff(win_editor, COLOR_PAIR(2));

                            } else
                                mvwprintw(win_editor, l + 2, 2, "%2zu ", l + 1, sres.lines[l].text);

                            wprintw(win_editor, "| %-60s", sres.lines[l].text);
                        } 

                        wmove(win_editor, cursor_y + 2, cursor_x + line_offt);
                        wrefresh(win_editor);
                    }
                }
            }

            if (dired_focus == 1) {

                wmove(win_dired, dy + 1, mk_len + 4);
                wrefresh(win_dired);

            } else if (dired_focus == 2) {

                wmove(win_dired, dy + 3, sh_len + 4);
                wrefresh(win_editor);

            } else if (in_editor) {

                wmove(win_editor, cursor_y + 2, cursor_x + line_offt);
                wrefresh(win_editor);
            }

            continue;
        }

        if (c == KEY_MOUSE) {

            MEVENT event;

            if (getmouse(&event) == OK) {

                if (!event.y && event.x >= term_x - 12)
                    break;

                if (event.x < dired_width) {

                    int dy = term_y - 6;

                    if (event.y == dy + 1) {

                        dired_focus = 1;
                        curs_set(1);

                    } else if (event.y == dy + 3) {
                        
                        dired_focus = 2;
                        curs_set(1);

                    } else {

                        dired_focus = 0;
                        curs_set(0);
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
                                mvwprintw(win_editor, 0, 2, "Editor [%s] [<C-x> | <C-f>]", freq.path);

                                if (fres.success) {

                                    in_editor = true;
                                    strncpy(current_file, freq.path, __FILENAME_LEN_MAX__);
                                    cursor_y = 0;
                                    cursor_x = 0;

                                    forrange(size_t, l, 0, fres.nlines, 1)
                                        mvwprintw(win_editor, l + 2, 2, "%2zu | %s", l + 1, fres.lines[l].text);

                                    curs_set(1);
                                    wmove(win_editor, cursor_y + 2, cursor_x + line_offt);

                                } else {
                                    
                                    mvwprintw(win_editor, 2, 2, "Failed to open file");
                                    in_editor = false;
                                    curs_set(0);
                                }

                                wrefresh(win_editor);
                            }
                        }
                    }

                } else if (in_editor) {

                    dired_focus = 0;
                    cursor_y = event.y - 2;
                    cursor_x = event.x - dired_width - line_offt;

                    if (cursor_y < 0)
                        cursor_y = 0;

                    if (cursor_x < 0)
                        cursor_x = 0;

                    if (cursor_x >= __PACKET_LEN_MAX__)
                        cursor_x = __PACKET_LEN_MAX__ - 1;

                    wmove(win_editor, cursor_y + 2, cursor_x + line_offt);
                    wrefresh(win_editor);
                }
            }

        } else if (dired_focus > 0) {

            if (c == KEY_ENTER || c == '\n' || c == '\r') {

                if (dired_focus == 1 && mk_len > 0) {

                    MkFileRequest mreq = {.type = PKT_MKFILE_REQ};
                    strncpy(mreq.name, mk_buf, __FILENAME_LEN_MAX__);
                    send(server_fd, &mreq, sizeof mreq, 0);

                    MkFileResponse mres;
                    if (recv(server_fd, &mres, sizeof mres, MSG_WAITALL) > 0) {

                        if (mres.success) {

                            sprintf(msg, "File `%s` created", mreq.name);

                            mk_buf[0] = NIL;
                            mk_len = 0;
                            dired_focus = 0;
                            curs_set(0);

                            DirRequest req = {.type = PKT_DIR_REQ};
                            strncpy(req.username, username, __USERNAME_LEN_MAX__);
                            send(server_fd, &req, sizeof req, 0);

                            if (recv(server_fd, &dres, sizeof dres, MSG_WAITALL) > 0) {

                                wclear(win_dired);
                                box(win_dired, 0, 0);
                                mvwprintw(win_dired, 0, 2, "Dired");
                                
                                int file_y = 2;
                                forrange(size_t, i, 0, dres.nnodes, 1) {

                                    FileNode node = dres.nodes[i];
                                    wmove(win_dired, file_y++, 2);

                                    forrange(int, d, 0, node.depth, 1)
                                        wprintw(win_dired, " ");
                                    
                                    wprintw(win_dired, (node.type == NODE_DIR ? "v %s" : "  %s"), dres.nodes[i].name);
                                }
                            }

                        } else
                            strncpy(msg, mres.msg, __ERRMSG_LEN_MAX__);
                    }

                } else if (dired_focus == 2 && sh_len > 0 && mk_len > 0) {
                    
                    MkFileRequest mreq = {.type = PKT_MKFILE_REQ};
                    strncpy(mreq.name, mk_buf, __FILENAME_LEN_MAX__);
                    send(server_fd, &mreq, sizeof mreq, 0);
                    
                    MkFileResponse mres;
                    if (recv(server_fd, &mres, sizeof mres, MSG_WAITALL) > 0) {

                        FLinkRequest sreq = {.type = PKT_FLINK_REQ};
                        strncpy(sreq.filename, mk_buf, __FILENAME_LEN_MAX__);
                        strncpy(sreq.username, sh_buf, __USERNAME_LEN_MAX__);
                        send(server_fd, &sreq, sizeof sreq, 0);

                        FLinkResponse sres;
                        if (recv(server_fd, &sres, sizeof sres, MSG_WAITALL) > 0) {

                            if (sres.success) {

                                sprintf(msg, "File `%s` to `%s`", sreq.filename, sreq.username);

                                mk_buf[0] = NIL;
                                mk_len = 0;
                                sh_buf[0] = NIL;
                                sh_len = 0;
                                dired_focus = 0;
                                curs_set(0);
                            
                            } else
                                strncpy(msg, sres.msg, __ERRMSG_LEN_MAX__);
                        }
                    }
                }

            } else if (c == KEY_BACKSPACE || c == 127 || c == '\b') {

                if (dired_focus == 1 && mk_len > 0)
                    mk_buf[--mk_len] = NIL;

                if (dired_focus == 2 && sh_len > 0)
                    sh_buf[--sh_len] = NIL;

            } else if (isprint(c)) {

                if (dired_focus == 1 && mk_len < 20) {

                    mk_buf[mk_len++] = c;
                    mk_buf[mk_len] = NIL;
                }
                
                if (dired_focus == 2 && sh_len < 20) {

                    sh_buf[sh_len++] = c;
                    sh_buf[sh_len] = NIL;
                }
            }

        } else if (c == CTRL('f') && in_editor) {

            FlushRequest ffreq = {.type = PKT_FLUSH_REQ};
            strncpy(ffreq.path, current_file, __FILENAME_LEN_MAX__);
            send(server_fd, &ffreq, sizeof ffreq, 0);

            FlushResponse ffres;
            if (recv(server_fd, &ffres, sizeof ffres, MSG_WAITALL) > 0) {

                if (ffres.success)
                    mvwprintw(win_editor, 0, 2, "Editor [%s] [<C-x> | <C-f>] [Flushed]", current_file);

                else
                    mvwprintw(win_editor, 0, 2, "Editor [%s] [<C-x> | <C-f>] [Flush Failed]");

                wmove(win_editor, cursor_y + 2, cursor_x + line_offt);
                wrefresh(win_editor);
            }
        
        } else if (in_editor) {

            EditRequest ereq = {

                .type = PKT_EDIT_REQ,
                .line = cursor_y,
                .col = cursor_x,
                .c = c,
                .del = false,
                .clipid = pid,
            };
            strncpy(ereq.path, current_file, __FILENAME_LEN_MAX__);

            if (locked_lines[cursor_y] && locked_lines[cursor_y] != pid) {

                beep();
                continue;
            }

            if (c == KEY_BACKSPACE || c == 127 || c == '\b') {

                if (cursor_x > 0) {

                    ereq.del = true;
                    cursor_x--;
                    mvwaddch(win_editor, cursor_y + 2, cursor_x + line_offt, ' ');
                    wmove(win_editor, cursor_y + 2, cursor_x + line_offt);
                    wrefresh(win_editor);

                    send(server_fd, &ereq, sizeof ereq, 0);
                }

            } else if (isprint(c)) {

                if (cursor_x < __PACKET_LEN_MAX__ - 1) {

                        mvwaddch(win_editor, cursor_y + 2, cursor_x + line_offt, c);
                        cursor_x++;
                        wmove(win_editor, cursor_y + 2, cursor_x + line_offt);
                        wrefresh(win_editor);
                        
                        send(server_fd, &ereq, sizeof ereq, 0);
                }
            }
        }
    }

    delwin(win_dired);
    delwin(win_editor);
}


int show_homepage(Role role) {

    mousemask(ALL_MOUSE_EVENTS, NULL);
    curs_set(0);
    int term_y, term_x;
    getmaxyx(stdscr, term_y, term_x);

    WINDOW* win = newwin(term_y, term_x, 0, 0);
    wtimeout(win, -1);
    keypad(win, TRUE);

    box(win, 0, 0);
    mvwprintw(win, 2, term_x / 2 - 10, "cmux session");
    mvwprintw(win, 5, term_x / 2 - 10, "[ View Files ]");
    mvwprintw(win, 7, term_x / 2 - 10, (role == ADMIN ? "[ View Users]" : "[ View Account ]"));
    mvwprintw(win, 9, term_x / 2 - 10, "[ Logout ]");
    wrefresh(win);

    int result = -1;
    int c;
    while ((c = wgetch(win))) {

        if (c == KEY_MOUSE) {

            MEVENT event;
            if (getmouse(&event) == OK) {

                if (term_x / 2 - 10 <= event.x && event.x <= term_x / 2 + 4) {

                    switch (event.y) {

                        case 5:
                            result = 1; 
                            goto done; 

                        case 7:
                            result = 2;
                            goto done;

                        case 9:
                            result = 0;
                            goto done;

                        default:
                            break;
                    }
                }
            }
        }
    }

    done:
        delwin(win);
        return result;
}



void show_admin_users(int server_fd) {

    mousemask(ALL_MOUSE_EVENTS, NULL);
    int term_y, term_x;
    getmaxyx(stdscr, term_y, term_x);
    WINDOW* win = newwin(term_y, term_x, 0, 0);
    wtimeout(win, 50);
    keypad(win, TRUE);

    char buf[__USERNAME_LEN_MAX__] = {NIL};
    int len = 0;
    bool focused = false;
    char msg[__ERRMSG_LEN_MAX__] = {NIL};

    int c;
    while ((c = wgetch(win))) {

        if (c == ERR) {

            UsersRequest ureq = {.type = PKT_USERS_REQ};
            send(server_fd, &ureq, sizeof ureq, 0);

            UsersResponse* ures = malloc(sizeof(UsersResponse));
            if (recv(server_fd, ures, sizeof(UsersResponse), MSG_WAITALL) > 0) {

                wclear(win);
                box(win, 0, 0);

                mvwprintw(win, 0, term_x - 12, "[< Back ]");
                mvwprintw(win, 2, 4, "%-20s | %-20s | %s", "Username", "Password", "Files Owned");
                mvwprintw(win, 3, 4, "---------------------------------------------------------------");

                forrange(size_t, i, 0, ures->nusers, 1) {

                    int nfiles = 0;
                    
                    forrange(int, f, 0, __FILES_OWNED_MAX__, 1) {

                        if (ures->users[i].files[f][0])
                            nfiles++;

                        }

                        mvwprintw(win, i + 4, 4, "%-20s | %-20s | %d", ures->users[i].username, ures->users[i].password, nfiles);
                    }

                int bottom_y = term_y - 4;
                mvwprintw(win, bottom_y, 4, "Username: [ %-20s ]", buf);
                mvwprintw(win, bottom_y, 38, "[ Add ]");
                mvwprintw(win, bottom_y, 48, "[ Remove ]");

                if (msg[0])
                    mvwprintw(win, bottom_y + 1, 4, "%s", msg);

                wrefresh(win);

                free(ures);
                continue;
            }
        }

        if (c == KEY_MOUSE) {

            MEVENT event;
            if (getmouse(&event) == OK) {

                if (!event.y && event.x >= term_x - 12)
                    break;
                    
                int bottom_y = term_y - 4;
                if (event.y == bottom_y && 16 <= event.x && event.x <= 36) {

                    focused = true;
                    curs_set(1);

                } else if (event.y == bottom_y && 38 <= event.x && event.x <= 45) {

                    AddUserRequest ureq = {.type = PKT_ADDUSER_REQ};
                    strncpy(ureq.username, buf, __USERNAME_LEN_MAX__);
                    send(server_fd, &ureq, sizeof ureq, 0);

                    AddUserResponse ures;
                    if (recv(server_fd, &ures, sizeof ures, MSG_WAITALL) > 0)
                        strncpy(msg, ures.msg, __ERRMSG_LEN_MAX__);
                    
                    else
                        msg[0] = NIL;

                    buf[0] = NIL;
                    len = 0;
                    focused = false;
                    curs_set(0);

                } else if (event.y == bottom_y && 48 <= event.x && event.x <= 58) {

                    RmUserRequest ureq = {.type = PKT_RMUSER_REQ};
                    strncpy(ureq.username, buf, __USERNAME_LEN_MAX__);
                    send(server_fd, &ureq, sizeof ureq, 0);

                    RmUserResponse ures;
                    if (recv(server_fd, &ures, sizeof ures, MSG_WAITALL) > 0) {

                        if (ures.success)
                            msg[0] = NIL;

                        else
                            strncpy(msg, ures.msg, __ERRMSG_LEN_MAX__);

                        buf[0] = 0;
                        len = 0;
                        focused = false;
                        curs_set(0);
                    }

                } else {

                    focused = false;
                    curs_set(0);
                }
            }

        } else if (focused) {

            if (c == KEY_BACKSPACE || c == 127 || c == '\b') {

                if (len > 0)
                    buf[--len] = NIL;

            } else if (isprint(c) && len < 20) {

                buf[len++] = c;
                buf[len] = NIL;
            }
        }
    }

    delwin(win);
}


void show_client_account(int server_fd, const char* username) {

    int term_y, term_x;
    getmaxyx(stdscr, term_y, term_x);
    WINDOW* win = newwin(term_y, term_x, 0, 0);
    wtimeout(win, 50);
    keypad(win, TRUE);

    char buf[__PASSWORD_LEN_MAX__] = {NIL};
    int len = 0;
    bool focused = false;
    char msg[100] = {NIL};

    int c;
    while ((c = wgetch(win))) {

        if (c == ERR) {

            UsersRequest ureq = {.type = PKT_USERS_REQ};
            send(server_fd, &ureq, sizeof ureq, 0);

            UsersResponse* ures = malloc(sizeof(UsersResponse));
            if (recv(server_fd, ures, sizeof(UsersResponse), MSG_WAITALL) > 0) {

                wclear(win);
                box(win, 0, 0);
                mvwprintw(win, 0, term_x - 12, "[< Back ]");
                mvwprintw(win, 2, 4, "Account Details");

                forrange(size_t, i, 0, ures->nusers, 1) {

                    if (!strncmp(ures->users[i].username, username, __USERNAME_LEN_MAX__)) {

                        mvwprintw(win, 4, 4, "Username: %s", ures->users[i].username);
                        mvwprintw(win, 5, 4, "Password: %s", ures->users[i].password);
                        break;
                    }
                }
                
                mvwprintw(win, 8, 4, "Change Password: [ %-20s ]", buf);
                mvwprintw(win, 8, 48, "[ v ]");

                if (msg[0])
                    mvwprintw(win, 10, 4, "%s", msg);

                wrefresh(win);
            }

            free(ures);
            continue;
        }

        if (c == KEY_MOUSE) {

            MEVENT event;
            if (getmouse(&event) == OK) {

                if (!event.y && event.x >= term_x - 12)
                    break;

                if (event.y == 8 && 23 <= event.x && event.x <= 45) {

                    focused = true;
                    curs_set(1);

                } else if (event.y == 8 && 48 <= event.x && event.x <= 52) {

                    ChPswdRequest creq = {.type = PKT_CHPSWD_REQ};
                    strncpy(creq.username, username, __USERNAME_LEN_MAX__);
                    strncpy(creq.pswd, buf, __PASSWORD_LEN_MAX__);
                    send(server_fd, &creq, sizeof creq, 0);

                    ChPswdResponse cres;
                    if (recv(server_fd, &cres, sizeof cres, MSG_WAITALL) > 0) {

                        strcpy(msg, cres.success ? "Password updated" : "Password Update Failed");
                        buf[0] = NIL;
                        len = 0;
                        focused = false;
                        curs_set(0);
                    }

                } else {

                    focused = false;
                    curs_set(0);
                }
            }

        } else if (focused) {

            if (c == KEY_BACKSPACE || c == 127 || c == '\b') {

                if (len > 0)
                    buf[--len] = NIL;

            } else if (isprint(c) && len < 20) {

                buf[len++] = c;
                buf[len] = NIL;
            }
        }
    }

    delwin(win);
}






