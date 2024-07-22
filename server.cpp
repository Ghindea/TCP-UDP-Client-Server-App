#include "lib.h"

unordered_map<string, _tcp_client_> tcp_clients;
unordered_set<int> connected_clients;
unordered_map<string, map<int, _tcp_client_ *>> topics; // <topic, <client_id, client>>

class Trie {
public:
    trie_node *root;

    Trie() {
        root = new trie_node();
    }
    void insert_key(string key) {
        trie_node *current = root;
        string pref = "";
        for (int i = 0; i < key.size(); i++) {
            char c = key[i];
            pref += c;
            int index = c - FIRST_LETTER;
            if (current->children[index] == nullptr) {
                current->children[index] = new trie_node(pref);
            }
            current = current->children[index];
        }
        current->isEnd = true;
    }

};

void decipher_content(_udp_message_ *message) {
    switch ((int) ((*message).tip_date)) {
        case 0 : {
            int value = 0;
            for (int i = 1; i <= 4; i++) {
                value = value * 256 + (unsigned char) (*message).content[i];
            }
            if ((*message).content[0] == 1) {
                value = -value;
            }
            strcpy((*message).content, to_string(value).c_str());
            break;
        }
        case 1 : {
            uint16_t abs = (uint8_t)(*message).content[0] * 256 + (uint8_t)(*message).content[1];
            float value = (float) abs / 100.00;

            std::ostringstream write_stream;
            write_stream << fixed << setprecision(2) << value;

            strcpy((*message).content, write_stream.str().c_str());
            break;
        }
        case 2 : {
            int value = 0;
            for (int i = 1; i <= 4; i++) {
                value = value * 256 + (unsigned char) (*message).content[i];
            }
            if ((*message).content[0] == 1) {
                value = -value;
            }
            float exp = pow(10, (uint8_t) (*message).content[5] * (-1));
            float result = (float) value * exp;

            strcpy((*message).content, to_string(result).c_str());
            break;
        }
    }
}

int main(int argc, char **argv) {

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);


    if (argc < 2) {
        cout << "Usage: ./server <PORT>" << '\n';
        return 0;
    }

    int rc; // return code - the savior of the day
    char buff[BUFLEN];
    socklen_t socklen = sizeof(struct sockaddr);
    uint16_t server_port;
    rc = sscanf(argv[1], "%hu", &server_port);
    DIE(rc < 0, "Given port is invalid");

    // region UDP listen socket
    const int udp_listen_fd = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(udp_listen_fd < 0, "socket");

    struct sockaddr_in udp_addr;
    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_port = htons(server_port);
    udp_addr.sin_addr.s_addr = INADDR_ANY;

    rc = bind(udp_listen_fd, (struct sockaddr *)&udp_addr, sizeof(udp_addr));
    DIE(rc < 0, "bind");
    //endregion

    // region TCP listen socket
    int tcp_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(tcp_listen_fd < 0, "socket");

    struct sockaddr_in tcp_addr;
    memset(&tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_port = htons(server_port);
    tcp_addr.sin_addr.s_addr = INADDR_ANY;

    rc = bind(tcp_listen_fd, (struct sockaddr *)&tcp_addr, sizeof(tcp_addr));
    DIE(rc < 0, "bind");
    // endregion

    // ascult pe tcp
    rc = listen(tcp_listen_fd, 50); // TODO check this 1
    DIE(rc < 0, "listen");

    // region Poll (multiplexare)
    struct pollfd *fds;
    fds = (struct pollfd *)malloc(INIT_POLL_SIZE * sizeof(struct pollfd));
    DIE(fds == NULL, "malloc");
    int poll_count = 0;

    fds[poll_count].fd = STDIN_FILENO;   // stdin
    fds[poll_count].events = POLLIN;
    poll_count++;

    fds[poll_count].fd = udp_listen_fd;  // udp socket
    fds[poll_count].events = POLLIN;
    poll_count++;

    fds[poll_count].fd = tcp_listen_fd;  // tcp socket
    fds[poll_count].events = POLLIN;
    poll_count++;

    int poll_size = INIT_POLL_SIZE;
    // endregion

    bool running = true;
    while (running) {
        // I have reached the end of the array, I must reallocate
        if (poll_count == poll_size) {
            fds = (struct pollfd *)realloc(fds, 2 * poll_size * sizeof(struct pollfd));
            DIE(fds == NULL, "realloc");
            poll_size *= 2;
        }
        
        rc = poll(fds, poll_count, 2000);
        DIE(rc < 0, "poll");

        for (int i = 0; i < poll_count; i++) {
            if (fds[i].revents & POLLIN) {
                if (fds[i].fd == STDIN_FILENO) {            // stdin
                    char buffer[BUFLEN];
                    fgets(buffer, sizeof(buffer), stdin);
                    buffer[strlen(buffer) - 1] = '\0';
                    if (strcmp(buffer, "exit") == 0) {
                        running = false;
                        break;
                    }

                } else if (fds[i].fd == udp_listen_fd) {    // udp input
                    memset(buff, 0, sizeof(buff));
                    rc = recvfrom(udp_listen_fd, buff, sizeof(buff), 0,
                                  (struct sockaddr *)&udp_addr, &socklen);
                    DIE(rc < 0, "recv");

                    message_to_tcp got_message;
                    got_message.signal_from_server = NOTIFICATION;
                    inet_ntop(AF_INET, &(udp_addr.sin_addr), got_message.recv_ip, sizeof(got_message.recv_ip));
                    got_message.recv_port = ntohs(udp_addr.sin_port);

                    _udp_message_ udp_message;
                    memcpy(&udp_message.topic, buff, 50); // topic
                    memcpy(&udp_message.tip_date, buff + 50, 1); // tip_date
                    memcpy(&udp_message.content, buff + 51, BUFLEN - 51); // content

                    decipher_content(&udp_message);
                    memcpy(&got_message.udp_message, &udp_message, sizeof(_udp_message_));

                    for (pair<int, _tcp_client_ *> subscriber : topics[udp_message.topic]) {
                        if (subscriber.second->status) {
                            rc = send(subscriber.second->client_socket, &got_message, sizeof(message_to_tcp), 0);
                            DIE(rc < 0, "send");
                        }
                    }
                } else if (fds[i].fd == tcp_listen_fd) {    // tcp connection
                    struct sockaddr_in new_tcp_addr;
                    socklen_t new_tcp_addr_len = sizeof(struct sockaddr_in);

                    int new_fd = accept(tcp_listen_fd,
                                        (struct sockaddr *)&new_tcp_addr,
                                        &new_tcp_addr_len);
                    DIE(new_fd < 0, "accept");

                    // disable Nagle's algorithm
                    int neagle = 1;
                    rc = setsockopt(new_fd, IPPROTO_TCP, TCP_NODELAY,
                                    &neagle, sizeof(int));
                    DIE(rc < 0, "setsockopt");

                    // reuse socket
                    int optval = 1;
                    rc = setsockopt(new_fd, SOL_SOCKET, SO_REUSEADDR,
                                    &optval, sizeof(optval));
                    DIE(rc < 0, "setsockopt");

                    // Enable SO_REUSEADDR option
                    optval = 1;
                    if (setsockopt(new_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
                        perror("setsockopt");
                        return 1;
                    }
                    DIE(rc < 0, "setsockopt");

                    memset(buff, 0, sizeof(buff));
                    rc = recv(new_fd, buff, sizeof(buff), 0);
                    DIE(rc < 0, "recv");

                    // client id already connected
                    if (tcp_clients.find(string(buff)) != tcp_clients.end()) {
                        if (tcp_clients[string(buff)].status) {
                            message_to_tcp exit_message;
                            exit_message.signal_from_server = EXIT;
                            rc = send(new_fd, &exit_message, sizeof(message_to_tcp), 0);
                            DIE(rc < 0, "send");

                            cout << "Client " << buff << " already connected.\n";
                            continue;
                        } else {
                            tcp_clients[string(buff)].status = true;
                            tcp_clients[string(buff)].client_socket = new_fd;
                            connected_clients.insert(new_fd);

                            cout << "New client " << buff << " connected from "
                                 << inet_ntoa(new_tcp_addr.sin_addr)
                                 << ":" << ntohs(new_tcp_addr.sin_port) << ".\n";
                        }
                    } else {
                        fds[poll_count].fd = new_fd;
                        fds[poll_count].events = POLLIN;
                        poll_count++;

                        _tcp_client_ new_client;
                        strcpy(new_client.client_id, buff);
//                        new_client.client_id = atoi(buff);
                        new_client.client_socket = new_fd;
                        new_client.status = true; // connected
                        tcp_clients.insert(make_pair(string(buff), new_client));
                        connected_clients.insert(new_fd);

                        cout << "New client " << buff << " connected from "
                             << inet_ntoa(new_tcp_addr.sin_addr)
                             << ":" << ntohs(new_tcp_addr.sin_port) << ".\n";
                    }
                } else if (connected_clients.find(fds[i].fd) != connected_clients.end()) { // tcp input (client is connected)
                    message_to_server client_message;
                    rc = recv(fds[i].fd, &client_message, sizeof(client_message), 0);
                    DIE(rc < 0, "recv");

                    if (client_message.signal_to_server == EXIT) {
//                        tcp_clients[client_message.client_id].status = false;
                        connected_clients.erase(fds[i].fd);
                        for (pair<string, _tcp_client_> client : tcp_clients) {
                            if (client.second.client_socket == fds[i].fd) {
                                tcp_clients[client.first].status = false;
                                cout << "Client " << client.second.client_id << " disconnected.\n";
                                break;
                            }
                        }
                        close(fds[i].fd);
                        continue;
                    }
                    if (client_message.signal_to_server == SUBSCRIBE) {
                        _tcp_client_ *subscriber = &tcp_clients[client_message.client_id];
                        topics[client_message.topic].insert(make_pair(atoi(client_message.client_id), subscriber));

//                        cout << "Client C" << client_message.client_id << " subscribed to topic " << client_message.topic << ".\n";
                    }
                    if (client_message.signal_to_server == UNSUBSCRIBE) {
                        topics[client_message.topic].erase(atoi(client_message.client_id));

//                        cout << "Client C" << client_message.client_id << " unsubscribed from topic " << client_message.topic << ".\n";
                    }
                }
            }
        }

    }
    // closing server
    for (int client : connected_clients) {
        message_to_tcp exit_message;
        exit_message.signal_from_server = EXIT;
        rc = send(client, &exit_message, sizeof(message_to_tcp), 0);
        DIE(rc < 0, "send");

//        connected_clients.erase(client);
        close(client);
    }
    connected_clients.clear();
    tcp_clients.clear();

    shutdown(tcp_listen_fd, SHUT_RDWR);
    shutdown(udp_listen_fd, SHUT_RDWR);

    close(udp_listen_fd);
    close(tcp_listen_fd);
    free(fds);

    return 0;
}
