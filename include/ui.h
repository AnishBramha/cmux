#pragma once

#include "common.h"

void show_login_screen(string_view* username, string_view* password, const char* errmsg);
void show_dired(int server_fd, const char* username, pid_t pid);



