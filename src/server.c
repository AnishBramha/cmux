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

        char path[__FILENAME_LEN_MAX__];
        snprintf(path, __FILENAME_LEN_MAX__, "data/remote/%s/", lreq.username);
        createdirs(path);

        DirRequest dreq;
        if (recv(client_fd, &dreq, sizeof dreq, 0) > 0 && dreq.type == PKT_DIR_REQ) {

            printf("SERVER: Directory request received from client `%s`\n", lreq.username);

            DirResponse dres = {.type = PKT_DIR_RES, .nnodes = 0};
            scan_directory(path, &dres, 0);

            send(client_fd, &dres, sizeof dres, 0);
            printf("SERVER: Sent %d directory nodes to client `%s`\n", dres.nnodes, lreq.username);
        }

        todo(CRASH, "Implement editor");

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

    forrange(int, i, 0, __OPEN_FILES_MAX__, 1)
        shared_files[i].active = false;

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





