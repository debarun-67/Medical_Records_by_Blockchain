#ifndef PROTOCOL_H
#define PROTOCOL_H
#define CMD_PROPOSE_BLOCK "PROPOSE_BLOCK:"
#define CMD_BLOCK_VOTE    "BLOCK_VOTE:"


void protocol_dispatch(int client_socket, const char *message);

#endif
