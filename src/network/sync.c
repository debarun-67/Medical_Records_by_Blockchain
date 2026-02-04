#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "node.h"
#include "../blockchain/blockchain.h"

// start chain sync
void initiate_chain_sync()
{
    printf("[SYNC] Initiating chain synchronization...\n");

    // query peers for height
    broadcast_message("GET_HEIGHT\n");
}

// request full chain download
void force_full_resync(int client_socket)
{
    printf("[SYNC] Forcing full chain resynchronization...\n");

    int local_height = get_blockchain_height();

    for (int i = 0; i < local_height; i++)
    {
        char request[64];
        snprintf(request, sizeof(request),
                 "GET_BLOCK:%d\n", i);

        send(client_socket, request, strlen(request), 0);
    }
}

// resolve sync conflict
void handle_sync_mismatch(int client_socket)
{
    printf("[SYNC] Chain mismatch detected. Requesting updated height.\n");
    send(client_socket, "GET_HEIGHT\n", 11, 0);
}
