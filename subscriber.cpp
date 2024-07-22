#include "lib.h"

int main(int argc, char **argv) {

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    if (argc < 4) {
        cout << "Usage: ./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER>" << '\n';
        return 0;
    }

    int rc;
    char buff[BUFLEN];
    uint16_t PORT_SERVER;
    rc = sscanf(argv[3], "%hu", &PORT_SERVER);
    DIE(rc != 1 || PORT_SERVER < 1024, "Given port is invalid");

    string ID_CLIENT = argv[1];
    string IP_SERVER = argv[2];


    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT_SERVER);

    int tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(tcp_fd < 0, "socket");

    // disable Neagle's algorithm
    uint8_t neagle = 1;
    rc = setsockopt(tcp_fd, IPPROTO_TCP, TCP_NODELAY,
                    &neagle, sizeof(int));
    DIE(rc < 0, "setsockopt");

    // enable TCP Fast Open
    int enable = 1;
    rc = setsockopt(tcp_fd, IPPROTO_TCP, TCP_FASTOPEN,
                    &enable, sizeof(enable));
    DIE(rc < 0, "setsockopt");

    rc = connect(tcp_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    DIE(rc < 0, "connect");

    memset(buff, 0, sizeof(buff));
    strcpy(buff, ID_CLIENT.c_str());
    rc = send(tcp_fd, &buff, sizeof(buff), 0);
    DIE(rc < 0, "send");

    // region Poll (multiplexare)
    struct pollfd *fds;
    fds = (struct pollfd *)malloc(INIT_POLL_SIZE * sizeof(struct pollfd));
    DIE(fds == NULL, "malloc");
    int poll_count = 0;

    fds[poll_count].fd = STDIN_FILENO;   // stdin
    fds[poll_count].events = POLLIN;
    poll_count++;

    fds[poll_count].fd = tcp_fd;  // tcp socket
    fds[poll_count].events = POLLIN;
    poll_count++;

    int poll_size = INIT_POLL_SIZE;
    // endregion

    bool connected = true;

    while (connected) {
        if (poll_count == poll_size) {
            fds = (struct pollfd *)realloc(fds, 2 * poll_size * sizeof(struct pollfd));
            DIE(fds == NULL, "realloc");
            poll_size *= 2;
        }

        rc = poll(fds, poll_count, -1);
        DIE(rc < 0, "poll");

        for (int i = 0; i < poll_count; i++) {
            if (fds[i].revents & POLLIN) {
                if (fds[i].fd == STDIN_FILENO) {
                    char buffer[BUFLEN];
                    fgets(buffer, sizeof(buffer), stdin);
                    buffer[strlen(buffer) - 1] = '\0';
                    if (strcmp(buffer, "exit") == 0) {
                        message_to_server client_message;
                        client_message.signal_to_server = EXIT;
                        strcpy(client_message.client_id, ID_CLIENT.c_str());

                        rc = send(tcp_fd, &client_message, sizeof(client_message), 0);
                        DIE(rc < 0, "send");


                        connected = false;
                        break;
                    }
                    if (strstr(buffer, "unsubscribe")) {
                        message_to_server client_message;
                        client_message.signal_to_server = UNSUBSCRIBE;
                        strcpy(client_message.client_id, ID_CLIENT.c_str());
                        strcpy(client_message.topic, buffer + 12);

                        rc = send(tcp_fd, &client_message, sizeof(client_message), 0);
                        DIE(rc < 0, "send");

                        cout << "Unsubscribed from topic " << client_message.topic << ".\n";
                    } else if (strstr(buffer, "subscribe")) {
                        message_to_server client_message;
                        client_message.signal_to_server = SUBSCRIBE;
                        strcpy(client_message.client_id, ID_CLIENT.c_str());
                        strcpy(client_message.topic, buffer + 10);
//                        client_message.topic[strlen(buffer) - 9] = '\0';

                        rc = send(tcp_fd, &client_message, sizeof(client_message), 0);
                        DIE(rc < 0, "send");

                        cout << "Subscribed to topic " << client_message.topic << ".\n";
                    }
                } else if (fds[i].fd == tcp_fd) {
                    message_to_tcp server_recv;
                    memset(&server_recv, 0, sizeof(server_recv));
                    rc = recv(tcp_fd, &server_recv, sizeof(server_recv), 0);
                    DIE(rc < 0, "recv");

                    if (server_recv.signal_from_server == EXIT) {
                        connected = false;
                        break;
                    }
                    if (server_recv.signal_from_server == NOTIFICATION) {
                        cout << server_recv.toString() << '\n';
                    }
                }
            }
        }
    }
    shutdown(tcp_fd, SHUT_RDWR); // shutdown the socket
    close(tcp_fd);
    exit(EXIT_SUCCESS);
    return 0;
}