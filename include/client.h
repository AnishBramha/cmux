#pragma once

#include "common.h"
#include "auth.h"


int run_client_handler(string_view* username, string_view* password, char* errmsg, pid_t* pid, Role* role);




