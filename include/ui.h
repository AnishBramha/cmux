#pragma once

#include "common.h"
#include "auth.h"

void show_login_screen(string_view* username, string_view* password, const char* errmsg);
void show_dired(int server_fd, const char* username, pid_t pid);
int show_homepage(Role);
void show_admin_users(int server_fd);
void show_client_account(int server_fd, const char* username);


