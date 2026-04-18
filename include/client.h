#pragma once

#include "common.h"


void handle_client_connection(int client_fd);
void run_client_editor(string_view* username, string_view* password);




