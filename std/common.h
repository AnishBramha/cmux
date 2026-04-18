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
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/types.h>
#include "array.h"
#include "string_view.h"


#define __ERRMSG_LEN_MAX__ 256

#define NIL '\0'
#define loop for (;;)

#define forrange(type, idx, start, end, step) \
    for (type idx = start; idx < end; idx += step)


#define __PORT__ 8000
#define __IP_LOCAL__ "127.0.0.1"
#define __BACKLOG__ 64
#define __PACKET_LEN_MAX__ 1024

#define unused(var) (void)(var)

#undef unreachable
#define unreachable()                                                                                            \
                                                                                                                 \
    do {                                                                                                         \
                                                                                                                 \
        fprintf(stderr, "PANIC: Unreachable statement on line `%d` reached in file `%s`\n", __LINE__, __FILE__); \
        abort();                                                                                                 \
                                                                                                                 \
    } while (0);


#define todo(task, ...)                                                                                        \
                                                                                                          \
    do {                                                                                                  \
                                                                                                          \
        fprintf(stderr, "TODO: Pending task `%s` on line `%d` in file `%s`\n", task, __LINE__, __FILE__); \
        abort();                                                                                          \
                                                                                                          \
    } while (0);


#define panic(errmsg, ...)                                                 \
                                                                           \
    do {                                                                   \
                                                                           \
        char msg[__ERRMSG_LEN_MAX__];                                      \
        snprintf(msg, __ERRMSG_LEN_MAX__ - 1, errmsg, ##__VA_ARGS__);      \
        fprintf(stderr, "PANIC: %s | REASON: %s\n", msg, strerror(errno)); \
        abort();                                                           \
                                                                           \
    } while (0);



