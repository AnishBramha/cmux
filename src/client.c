#include "client.h"
#include "common.h"


static inline char* strrole(Role role) {

    return role == ADMIN ? "ADMIN" : "CLIENT";
}


int run_client_handler(string_view *username, string_view *password, char* errmsg, pid_t* pid, Role* role) {

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        panic("CLIENT [%d]: Failed to open socket", getpid());
    }

    struct sockaddr_in server_addr = {

        .sin_family = AF_INET,
        .sin_port = htons(__PORT__),
    };

    if (inet_pton(AF_INET, __IP_LOCAL__, &server_addr.sin_addr) <= 0) {
        panic("CLIENT [%d]: IP Address/Port error", getpid());
    }

    printf("CLIENT [%d]: Connecting to server at INET %s | PORT %d\n", getpid(), __IP_LOCAL__, __PORT__);

    if (connect(sock_fd, (struct sockaddr*)&server_addr, (socklen_t)(sizeof server_addr)) == -1) {
        panic("CLIENT [%d]: Connection to server at INET %s | PORT %d failed\n", getpid(), __IP_LOCAL__, __PORT__);
    }

    printf("CLIENT [%d]: Connected to server at INET %s | PORT %d\n", getpid(),  __IP_LOCAL__, __PORT__);
    printf("CLIENT [%d]: Authenticating login for\nUsername: "sv_fmt"\nPassword: "sv_fmt"\n", getpid(), sv_arg(*username), sv_arg(*password));

    LoginRequest lreq = {.type = PKT_LOGIN_REQ};
    strncpy(lreq.username, username->str, username->len);
    strncpy(lreq.password, password->str, password->len);

    printf("CLIENT [%d]: Sending login request\n", getpid());
    if (send(sock_fd, &lreq, sizeof lreq, 0) < 0) {
        panic("CLIENT [%d]: Failed to send login request\n", getpid());
    }

    LoginResponse lres;
    if (recv(sock_fd, &lres, sizeof lres, 0) <= 0) {
        panic("CLIENT [%d]: Login response dropped", getpid());
    }

    if (lres.success) {

        printf("CLIENT [%d]: Logged in with role `%s`\n", getpid(), strrole(lres.role));
        *pid = getpid();
        *role = lres.role;
        return sock_fd;

    } else {

        fprintf(stderr, "PANIC: CLIENT [%d]: Login failed | REASON: %s\n", getpid(), lres.msg);
        strcpy(errmsg, lres.msg);
        close(sock_fd);

        return -1;
    }
}



