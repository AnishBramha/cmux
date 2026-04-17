#pragma once

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <ncurses.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/select.h>
#include "array.h"
#include "string_view.h"


#define NIL '\0'

#define loop for (;;)

#define unused(var) (void)(var)

#define unreachable()                                                                                            \
                                                                                                                 \
    do {                                                                                                         \
                                                                                                                 \
        fprintf(stderr, "PANIC: Unreachable statement on line `%d` reached in file `%s`\n", __LINE__, __FILE__); \
        abort();                                                                                                 \
                                                                                                                 \
    } while (0);


#define todo(task)                                                                                        \
                                                                                                          \
    do {                                                                                                  \
                                                                                                          \
        fprintf(stderr, "TODO: Pending task `%s` on line `%d` in file `%s`\n", task, __LINE__, __FILE__); \
        abort();                                                                                          \
                                                                                                          \
    } while (0);



