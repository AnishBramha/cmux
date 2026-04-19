#include "common.h"
#include "ui.h"
#include "auth.h"
#include "server.h"
#include "client.h"
#include <sysexits.h>


int main(int argc, char** argv) {

    if (argc >= 2 && !strcmp(argv[1], "--launch")) {

        puts("MAIN: Launching cmux server daemon");
        run_server_daemon();

        return EXIT_SUCCESS;
    }

    string_view username = {.str = (char[__USERNAME_LEN_MAX__]){NIL}, .len = 0};
    string_view password = {.str = (char[__PASSWORD_LEN_MAX__]){NIL}, .len = 0};
    char errmsg[__ERRMSG_LEN_MAX__] = {NIL};

    int server_sock_fd = -1;
    pid_t clipid;
    Role clirole;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    start_color();
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_WHITE, COLOR_RED);
    init_pair(3, COLOR_RED, COLOR_WHITE);

    loop {

        clear();
        refresh();

        show_login_screen(&username, &password, errmsg);

        endwin();

        server_sock_fd = run_client_handler(&username, &password, errmsg, &clipid, &clirole);
        if (server_sock_fd != -1)
            break;

        usleep(1000);
    }

    bool running = true;
    while (running) {
        
        clear();
        refresh();

        int choice = show_homepage(clirole);
        switch (choice) {

            case 0:
                running = false;
                break;

            case 1:
                show_dired(server_sock_fd, username.str, clipid);
                break;
                
            case 2:

                if (clirole == ADMIN)
                    show_admin_users(server_sock_fd);

                else
                    show_client_account(server_sock_fd, username.str);

                break;

            default:
                break;
        }
    }

    endwin();
    
    close(server_sock_fd);
    return EXIT_SUCCESS;
}




