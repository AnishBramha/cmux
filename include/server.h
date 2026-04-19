#pragma once

#include <pthread.h>
#include "auth.h"
#include "common.h"

typedef struct {

    bool active;
    char path[__FILENAME_LEN_MAX__];
    size_t nlines;
    pthread_mutex_t edit_lock;
    Line lines[__FILE_LINES_MAX__];

} ActiveFile;

extern size_t* nusers;
extern Record* users;
extern ActiveFile* shared_files;

void run_server_daemon(void);
void reap_zombie(int);
void handle_client_connection(int client_fd);
void load_shm(void);
void scan_directory(const char* path, DirResponse* dres, int depth);









