#include "client.h"
#include "common.h"


void handle_client_connection(int client_fd) {

    char buf[__PACKET_LEN_MAX__];
    while (recv(client_fd, buf, sizeof buf, 0) > 0)
        todo("CLIENT: Implement login and editor\n");
}


void run_client_editor(string_view *username, string_view *password) {

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
    todo("Authenticate and login client");
}



