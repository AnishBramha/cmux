#include "common.h"
#include "main_ui.h"
#include "auth.h"
#include <sysexits.h>


int main(void) {

    string_view username = {.str = (char[__USERNAME_LEN_MAX__]){0}, .len = 0};
    string_view password = {.str = (char[__PASSWORD_LEN_MAX__]){0}, .len = 0};
    show_login_screen(&username, &password);

    printf("Username: "sv_fmt"\nPassword: "sv_fmt"\n", sv_arg(username), sv_arg(password));


    if (!strncmp(username.str, __USERNAME_ADMIN__, username.len)) {

        if (!strncmp(password.str, __PASSWORD_ADMIN__, password.len)) {

            todo("Handle Admin");

        } else {

            todo("Handle Illegal Admin Entry");
        }

    } else {

        todo("Handle client entry");
    }

    return 0;
}




