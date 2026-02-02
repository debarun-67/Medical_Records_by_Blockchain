#ifndef NODE_H
#define NODE_H

#include <time.h>

#define MAX_PEERS 50
#define BUFFER_SIZE 2048

typedef struct {
    int socket;
    int port;
    time_t last_seen;
    int active;
} Peer;

void start_server(int port);
void connect_to_peer(const char *ip, int port);
void broadcast_message(const char *message);
void send_message(int socket, const char *message);
int get_peer_count();
void update_peer_last_seen(int socket);

#endif
