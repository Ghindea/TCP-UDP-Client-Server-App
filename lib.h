#include <bits/stdc++.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <map>


using namespace std;

#define DIE(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
      perror(call_description);                                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

#define INIT_POLL_SIZE 32
#define MAX_IP_LEN 10
#define MAX_TOPIC_LEN 51
#define BUFLEN 1551
#define ALPHABET_SIZE '~' - '!' + 1
#define FIRST_LETTER '!'

struct _udp_message_ {
    char content[BUFLEN], topic[MAX_TOPIC_LEN];
    uint8_t tip_date;
};

struct _tcp_client_ {
    char client_id[MAX_IP_LEN];
    int client_socket;
    uint16_t client_port;

    bool status;    // connected or not
};
enum server_signal {
    SUBSCRIBE,
    UNSUBSCRIBE,
    NOTIFICATION,
    EXIT
};
struct message_to_tcp {
    server_signal signal_from_server;
    uint16_t recv_port;
    char recv_ip[MAX_IP_LEN];
    _udp_message_ udp_message;

    string toString() {
        string s = "";
        s +=  udp_message.topic +
                string(" - ");
        switch (udp_message.tip_date) {
            case 0 : {
                s += "INT - "; break;
            }
            case 1 : {
                s += "SHORT_REAL - "; break;
            }
            case 2 : {
                s += "FLOAT - "; break;
            }
            case 3 : {
                s += "STRING - ";break;
            }
        }
        s += udp_message.content;

        return s;
    }
};
struct message_to_server {
    server_signal signal_to_server;

    char client_id[MAX_IP_LEN];
    char topic[MAX_TOPIC_LEN];
};
struct trie_node {
    struct trie_node* children[ALPHABET_SIZE]{};
    bool isEnd;
    string word;

    trie_node() {
        isEnd = false;
        word = "";
        for (trie_node *child : children) {
            child = nullptr;
        }
    }
    trie_node(string word) {
        trie_node();
        this->word = word;
    }
};