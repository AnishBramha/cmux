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

    show_login_screen(&username, &password);
    run_client_editor(&username, &password);

    return 0;
}




