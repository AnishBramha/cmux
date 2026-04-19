#include "server.h"
#include "common.h"
#include "auth.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <dirent.h>
#include <sys/stat.h>


static inline char* strrole(Role role) {

    return role == ADMIN ? "ADMIN" : "CLIENT";
}


size_t* nusers = NULL;
Record* users = NULL;
ActiveFile* shared_files = NULL;


void run_server_daemon(void) {

    signal(SIGPIPE, SIG_IGN);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        panic("SERVER: Failed to open file descriptor");
    }

    int enable = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1) {
        panic("SERVER: Failed to reuse address");
    }

    struct sockaddr_in server_addr = {

        .sin_family = AF_INET,
        .sin_port = htons(__PORT__),
        .sin_addr.s_addr = INADDR_ANY,
        .sin_zero = {NIL},
    };

    if (bind(server_fd, (struct sockaddr*)&server_addr, (socklen_t)sizeof(struct sockaddr)) == -1) {
        panic("SERVER: Failed to bind socket to port %d", __PORT__);
    }

    if (listen(server_fd, __BACKLOG__) == -1) {
        panic("SERVER: Failed to start listening on port %d\n", __PORT__);
    }

    struct sigaction zombie_reaper = {

        .sa_handler = reap_zombie,
        .sa_flags = SA_RESTART,
    };
    sigemptyset(&zombie_reaper.sa_mask);
    sigaction(SIGCHLD, &zombie_reaper, NULL);

    load_shm();

    loop {

        printf("SERVER: Listening on port %d\n", __PORT__);

        struct sockaddr_in client_addr;
        size_t client_addrlen = sizeof client_addr;
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, (socklen_t*)&client_addrlen);
        if (client_fd == -1) {

            fputs("SERVER: Client connection failed\n", stderr);
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);

        pid_t pid = fork();
        if (pid < 0) {
            panic("SERVER: Failed to fork a child handler");
        }

        if (!pid) {

            close(server_fd);

            printf("CLIENT [%d]: Accepted connection from INET %s | PORT %d\n", getpid(), client_ip, __PORT__);

            handle_client_connection(client_fd);

            printf("CLIENT [%d]: Closing connection from INET %s | PORT %d\n", getpid(), client_ip, __PORT__);
            close(client_fd);
            exit(EXIT_SUCCESS);

        } else {

            printf("SERVER: Connected [INET %s] to client [%d]\n", client_ip, pid);
            close(client_fd);
        }
    }
}




void reap_zombie(int sig) {

    unused(sig);
    while (waitpid(-1, NULL, WNOHANG) > 0);
}




static void createdirs(const char* fpath) {

    char path[__FILENAME_LEN_MAX__];
    strncpy(path, fpath, __FILENAME_LEN_MAX__ - 1);

    for (int i = 0; path[i]; i++) {

        if (path[i] == '/') {

            path[i] = NIL;
            if (mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO) && errno != EEXIST)
                panic("SERVER: Failed to create directory `%s`", path);

            path[i] = '/';
        }
    }
}




void handle_client_connection(int client_fd) {

    LoginRequest lreq;
    if (recv(client_fd, &lreq, sizeof lreq, 0) <= 0) {

        fputs("SERVER: Client did not send login details\n", stderr);
        return;
    }

    if (lreq.type != PKT_LOGIN_REQ) {

        fputs("SERVER: Expected login request, but got garbage\n", stderr);
        return;
    }

    LoginResponse lres = {.type = PKT_LOGIN_RES, .role = CLIENT, .success = false};
    
    if (!strncmp(lreq.username, __USERNAME_ADMIN__, __USERNAME_LEN_MAX__)) {
        
        if (!strncmp(lreq.password, __PASSWORD_ADMIN__, __PASSWORD_LEN_MAX__)) {

            lres.success = true;
            lres.role = ADMIN;
            strcpy(lres.msg, "Admin access granted");

        } else
            strcpy(lres.msg, "Illegal admin password");

    } else {

        forrange(size_t, i, 0, *nusers, 1) {

            if (!strncmp(lreq.username, users[i].username, __USERNAME_LEN_MAX__)) {

                if (!strncmp(lreq.password, users[i].password, __PASSWORD_LEN_MAX__))
                    lres.success = true;

                else
                    sprintf(lres.msg, "Invalid password for user `%s`", users[i].username);

                goto done;
            }
        }

        strcpy(lres.msg, "Username not found");
    }

    done:
        send(client_fd, &lres, sizeof lres, 0);

    if (lres.success) {

        printf("SERVER: Client authentication for `%s` finished with role `%s`\n", lreq.username, strrole(lres.role));

        createdirs("data/remote/");

        DirRequest dreq;
        if (recv(client_fd, &dreq, sizeof dreq, 0) > 0 && dreq.type == PKT_DIR_REQ) {
            
            printf("SERVER: Directory request received from client `%s`\n", lreq.username);
            DirResponse dres = {.type = PKT_DIR_RES, .nnodes = 0};

            if (lres.role == ADMIN)
                scan_directory("data/remote", &dres, 0);
            
            else {

                forrange(size_t, i, 0, *nusers, 1) {

                    if (!strcmp(users[i].username, lreq.username)) {
                        
                        forrange(int, f, 0, __FILES_OWNED_MAX__, 1) {

                            if (users[i].files[f][0] != NIL) {

                                size_t idx = dres.nnodes++;
                                strncpy(dres.nodes[idx].name, users[i].files[f], __FILENAME_LEN_MAX__);

                                dres.nodes[idx].type = NODE_FILE;
                                dres.nodes[idx].depth = 0;
                            }
                        }
                        break;
                    }
                }
            }

            size_t total_sent = 0;
            char* ptr = (char*)&dres;

            while (total_sent < sizeof(dres)) {

                ssize_t s = send(client_fd, ptr + total_sent, sizeof(dres) - total_sent, 0);

                if (s <= 0)
                    break;
                
                total_sent += s;
            }

            printf("SERVER: Sent %d directory nodes to client `%s`\n", dres.nnodes, lreq.username);
        }

        loop {

            PacketType type;
            if (recv(client_fd, &type, sizeof(PacketType), MSG_PEEK) <= 0)
                break;

            if (type == PKT_FOPEN_REQ) {

                FOpenRequest freq;
                if (recv(client_fd, &freq, sizeof freq, MSG_WAITALL) <= 0)
                    break;

                printf("SERVER: File open request `%s` from client `%s`\n", freq.path, lreq.username);

                ActiveFile* fptr = NULL;
                forrange(int, i, 0, __OPEN_FILES_MAX__, 1) {

                    if (shared_files[i].active && !strcmp(shared_files[i].path, freq.path)) {
                        
                        fptr = &shared_files[i];
                        break;
                    }
                }

                if (!fptr) {

                    forrange(int, i, 0, __OPEN_FILES_MAX__, 1) {

                        if (!shared_files[i].active) {
                            
                            fptr = &shared_files[i];
                            break;
                        }
                    }

                    if (fptr) {

                        char path[__FILENAME_LEN_MAX__];
                        sprintf(path, "data/remote/%s", freq.path);

                        int fd = open(path, O_RDONLY);
                        if (fd == -1) {

                            fprintf(stderr, "SERVER: Failed to open file `%s` by client `%s`\n", path, lreq.username);
                            fptr = NULL;

                        } else {

                            fptr->active = true;
                            strncpy(fptr->path, freq.path, __FILENAME_LEN_MAX__);
                            fptr->nlines = 0;

                            char buffer[__PACKET_LEN_MAX__];
                            ssize_t bytes;
                            int col = 0;

                            while ((bytes = read(fd, buffer, sizeof buffer)) > 0) {

                                forrange(ssize_t, i, 0, bytes, 1) {

                                    char c = buffer[i];
                                    if (c == '\n') {

                                        fptr->lines[fptr->nlines++].text[col] = NIL;
                                        col = 0;
                                        
                                        if (fptr->nlines >= __FILE_LINES_MAX__)
                                            goto done2;

                                    } else {

                                        if (col < __PACKET_LEN_MAX__ - 1)
                                            fptr->lines[fptr->nlines].text[col++] = c;
                                    }
                                }
                            }

                            if (col > 0 && fptr->nlines < __FILE_LINES_MAX__)
                                fptr->lines[fptr->nlines++].text[col] = NIL;

                            done2:
                                close(fd);
                        }
                    }
                }

                FOpenResponse fres = {

                    .type = PKT_FOPEN_RES,
                    .success = false,
                    .nlines = 0,
                };

                if (fptr) {

                    fres.success = true;
                    fres.nlines = fptr->nlines;
                    
                    forrange(size_t, l, 0, fres.nlines, 1)
                        fres.lines[l] = fptr->lines[l];

                    size_t total_sent = 0;
                    char* ptr = (char*)&fres;

                    while (total_sent < sizeof fres) {

                        ssize_t s = send(client_fd, ptr + total_sent, sizeof fres - total_sent, 0);
                        
                        if (s <= 0)
                            break;
                            
                        total_sent += s;
                    }
                }

            } else if (type == PKT_EDIT_REQ) {

                EditRequest ereq;
                if (recv(client_fd, &ereq, sizeof ereq, MSG_WAITALL) <= 0)
                    break;

                ActiveFile* fptr = NULL;
                forrange(int, i, 0, __OPEN_FILES_MAX__, 1) {

                    if (shared_files[i].active && !strcmp(shared_files[i].path, ereq.path)) {

                        fptr = &shared_files[i];
                        break;
                    }
                }

                if (fptr) {

                    if (0 <= ereq.line  && ereq.line < __FILE_LINES_MAX__ && 0 <= ereq.col && ereq.col < __PACKET_LEN_MAX__ - 1) {

                        pthread_mutex_lock(&fptr->edit_lock);

                        if (fptr->lines[ereq.line].locked && fptr->lines[ereq.line].locked != ereq.clipid) {

                            pthread_mutex_unlock(&fptr->edit_lock);
                            goto skip_edit;
                        }
                        fptr->lines[ereq.line].locked = ereq.clipid;

                        if ((size_t)ereq.line >= fptr->nlines)
                            fptr->nlines = ereq.line + 1;

                        size_t len = strlen(fptr->lines[ereq.line].text);

                        if (ereq.del) {

                            if (ereq.col < len)
                                fptr->lines[ereq.line].text[ereq.col] = ' ';

                        } else {

                            while (len < ereq.col)
                                fptr->lines[ereq.line].text[len++] = ' ';

                            fptr->lines[ereq.line].text[ereq.col] = ereq.c;

                            if (ereq.col >= len)
                            fptr->lines[ereq.line].text[ereq.col + 1] = NIL;
                        }

                        pthread_mutex_unlock(&fptr->edit_lock);
                    }
                }
                skip_edit:;

            } else if (type == PKT_SYNC_REQ) {

                SyncRequest sreq;
                if (recv(client_fd, &sreq, sizeof sreq, MSG_WAITALL) <= 0)
                    break;

                SyncResponse sres = {

                    .type = PKT_SYNC_RES,
                    .success = false,
                    .nlines = 0,
                };

                forrange(int, i, 0, __OPEN_FILES_MAX__, 1) {

                    if (shared_files[i].active && !strcmp(shared_files[i].path, sreq.path)) {

                        sres.success = true;
                        sres.nlines = shared_files[i].nlines;

                        pthread_mutex_lock(&shared_files[i].edit_lock);

                        forrange(size_t, l, 0, __FILE_LINES_MAX__, 1) {

                            if (shared_files[i].lines[l].locked == sreq.clipid && (int)l != sreq.cursor_line)
                                shared_files[i].lines[l].locked = 0;
                        }

                        if (0 <= sreq.cursor_line && sreq.cursor_line < __FILE_LINES_MAX__) {

                            if (!shared_files[i].lines[sreq.cursor_line].locked)
                                shared_files[i].lines[sreq.cursor_line].locked = sreq.clipid;
                        }

                        forrange(size_t, l, 0, sres.nlines, 1)
                            sres.lines[l] = shared_files[i].lines[l];

                        pthread_mutex_unlock(&shared_files[i].edit_lock);
                        break;
                    }
                }

                size_t total_sent = 0;
                char* ptr = (char*)&sres;

                while (total_sent < sizeof sres) {

                    ssize_t s = send(client_fd, ptr + total_sent, sizeof sres - total_sent, 0);

                    if (s <= 0)
                        break;

                    total_sent += s;
                }
                
            } else if (type == PKT_FLUSH_REQ) {

                FlushRequest ffreq;
                if (recv(client_fd, &ffreq, sizeof ffreq, MSG_WAITALL) <= 0)
                    break;
                
                FlushResponse ffres = {.type = PKT_FLUSH_RES, .success = false};
                
                ActiveFile* fptr = NULL;
                forrange(int, i, 0, __OPEN_FILES_MAX__, 1) {

                    if (shared_files[i].active && !strcmp(shared_files[i].path, ffreq.path)) {
                        
                        fptr = &shared_files[i];
                        break;
                    }
                }

                if (fptr) {

                    char path[__FILENAME_LEN_MAX__];
                    sprintf(path, "data/remote/%s", ffreq.path);

                    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
                    if (fd == -1)
                        fprintf(stderr, "SERVER: Failed to flush file `%s` to disk from client `%s`\n", path, lreq.username);

                    else {

                        forrange(size_t, l, 0, fptr->nlines, 1) {

                            write(fd, fptr->lines[l].text, strlen(fptr->lines[l].text));
                            write(fd, "\n", 1);
                        }
                        
                        close(fd);
                        ffres.success = true;

                        printf("SERVER: Flushed file `%s` to disk from client `%s`\n", path, lreq.username);
                    }
                }

                size_t total_sent = 0;
                char* ptr = (char*)&ffres;

                while (total_sent < sizeof ffres) {

                    ssize_t s = send(client_fd, ptr + total_sent, sizeof ffres - total_sent, 0);
                    if (s <= 0)
                        break;

                    total_sent += s;
                }

            } else {

                char trash;
                recv(client_fd, &trash, 1, 0);
            }
        }

    } else
        fprintf(stderr, "SERVER: Authentication failed for client `%s`\n", lreq.username);
}





void load_shm(void) {

    size_t usr_memsize = sizeof(size_t) + sizeof(Record) * __USERS_MAX__;
    size_t files_memsize = sizeof(ActiveFile) * __OPEN_FILES_MAX__;
    size_t memsize = usr_memsize + files_memsize;
    void* mem = mmap(NULL, memsize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);

    if (mem == MAP_FAILED) {
        panic("SERVER: Failed to load shared memory for user database");
    }

    nusers = (size_t*)mem;
    *nusers = 0;
    users = (Record*)((char*)mem + sizeof(size_t));
    shared_files = (ActiveFile*)((char*)mem + usr_memsize);

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

    forrange(int, i, 0, __OPEN_FILES_MAX__, 1) {
        
        shared_files[i].active = false;
        pthread_mutex_init(&shared_files[i].edit_lock, &attr);
    }

    pthread_mutexattr_destroy(&attr);

    int fd = open(__USERS_DB__, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (fd == -1) {
        panic("SERVER: Failed to load user database");
    }

    ssize_t bytes = read(fd, users, sizeof(Record) * __USERS_MAX__);
    if (bytes > 0) {

        *nusers = bytes / sizeof(Record);
        printf("SERVER: Loaded %zu users from database\n", *nusers);

    } else if (!bytes)
        puts("SERVER: No users found in database");

    else {
        panic("SERVER: Read failed on users database\n");
    }

    close(fd);
}



void scan_directory(const char* path, DirResponse* dres, int depth) {

    if (dres->nnodes >= __FILES_OWNED_MAX__)
        return;

    DIR* dir = opendir(path);
    if (!dir)
        return;

    struct dirent* dentry;
    while ((dentry = readdir(dir))) {

        if (!strcmp(dentry->d_name, ".") || !strcmp(dentry->d_name, ".."))
            continue;

        size_t idx = dres->nnodes++;
        strncpy(dres->nodes[idx].name, dentry->d_name, __FILENAME_LEN_MAX__);
        dres->nodes[idx].depth = depth;

        char aug_path[__FILENAME_LEN_MAX__];
        sprintf(aug_path, "%s/%s", path, dentry->d_name);

        struct stat statbuf;
        if (!stat(aug_path, &statbuf) && S_ISDIR(statbuf.st_mode)) {

            dres->nodes[idx].type = NODE_DIR;
            scan_directory(aug_path, dres, depth + 1);

        } else
            dres->nodes[idx].type = NODE_FILE;
    }

    closedir(dir);
}





