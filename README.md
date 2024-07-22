### SERVER-CLIENT APPLICATION WITH UDP AND TCP

by: **_Daniel GHINDEA_** - 325CB

My homework implements a simple C++ application which allows UDP clients to
connect to a server that acts like a broker, communicating with TCP subscribers.

See the requirements [here](./Enunt_Tema_2_Protocoale_2023_2024.pdf)

## Description

This project is designed to implement a server-client model where the server can handle requests from multiple subscribers. It includes a simple communication system where the `server` component processes incoming connections and interactions, while `subscriber` acts as the client that connects to and communicates with the server.

## Features

- **Server Implementation**: Handles incoming connections and processes client requests.
- **Subscriber Client**: Connects to the server and sends requests based on user input.
- **Modular Design**: Core functionalities are encapsulated in the `lib.h` header.

### Building the Project

To build the server and the subscriber programs, you can use the provided `Makefile`:

```bash
make
```

This command compiles the `server` and `subscriber` programs and generates executable files.

### Running the Application

To run the server program:
```
./server <PORT>
```

To run the subscriber client in a separate terminal:
```
./subscriber <ID> <SERVER_IP> <SERVER_PORT>
```

To run all available tests:
```
python3 test.py
```

Ensure the server is running before starting the subscriber.

### About TCP protocol
- when server is launched it creates 3 sockets to communicate with STDIN, UDP clients & TCP clients
- when client starts it creates a socket for TCP connection with the server through which the ID of the client is sent
- to implement the multiplexation of connections the program uses poll
- server-side uses a special structure to communicate with the TCP-client (`message_to_tcp`) which can transmit the message that came from udp to that client & a custom signal to transmit actions

```cpp
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
            case 0 : {s += "INT - "; break;}
            case 1 : {s += "SHORT_REAL - "; break;}
            case 2 : {s += "FLOAT - "; break;}
            case 3 : {s += "STRING - ";break;}
        }
        s += udp_message.content;
        return s;
    }
};
```
- client-side uses another structure to communicate with the server (`message_to_server`) which can transmit data about the content that the client wants to see & also a custom signal to transmit actions

```cpp
struct message_to_server {
    server_signal signal_to_server;

    char client_id[MAX_IP_LEN];
    char topic[MAX_TOPIC_LEN];
};
```
- the custom signal is represented by an enum which describes 4 possible actions

```cpp
enum server_signal {
    SUBSCRIBE,		// sent by tcp client to tell the server it wants to subscribe
    UNSUBSCRIBE,	// sent by tcp client to tell the server it wants to unsubscribe
    NOTIFICATION,	// sent by the server to tell the client it has a new notification
    EXIT		// sent by both of them to tell to the other one that it's is disconnecting
};
```

**Note**: At the moment wilcards are not working.

RESULTS
```
compile...............................................passed
server_start..........................................passed
c1_start..............................................passed
data_unsubscribed.....................................passed
c1_subscribe_all......................................passed
data_subscribed.......................................passed
c1_stop...............................................passed
c1_restart............................................passed
data_no_clients.......................................passed
same_id...............................................passed
c2_start..............................................passed
c2_subscribe..........................................passed
c2_stop...............................................passed
server_stop...........................................passed
```