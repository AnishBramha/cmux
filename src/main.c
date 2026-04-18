#include "common.h"
#include "main_ui.h"
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

    loop {

        show_login_screen(&username, &password, errmsg);

        int server_sock_fd = run_client_handler(&username, &password, errmsg);
        if (server_sock_fd != -1)
            break;

        usleep(1000);
    }

    todo(CRASH, "Implement file picker");

    return 0;
}




