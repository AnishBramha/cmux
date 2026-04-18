#include "server.h"
#include "common.h"
#include "auth.h"
#include <signal.h>
#include <stdio.h>


static char* strrole(Role role) {

    return role == ADMIN ? "ADMIN" : "CLIENT";
}


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

        todo(DEBUG, "Implement client login");
    }

    send(client_fd, &lres, sizeof lres, 0);

    if (lres.success) {

        printf("SERVER: Client authentication finished with role `%s`\n", strrole(lres.role));

        todo(CRASH, "Implement start editor");
    }
}





