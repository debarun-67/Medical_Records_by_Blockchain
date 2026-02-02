#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include "node.h"
#include "../blockchain/blockchain.h"

/* =========================
   INITIAL SYNC
========================= */
void initiate_chain_sync()
{
    printf("Initiating chain sync...\n");

    /* Step 1: Ask all peers their height */
    broadcast_message("GET_HEIGHT\n");
}

/* =========================
   FORCE FULL RESYNC
========================= */
void force_full_resync(int client_socket)
{
    printf("Forcing full chain resync...\n");

    int local_height = get_blockchain_height();

    for (int i = 0; i < local_height; i++)
    {
        char request[64];
        snprintf(request, sizeof(request),
                 "GET_BLOCK:%d\n", i);

        send(client_socket, request, strlen(request), 0);
    }
}

/* =========================
   HANDLE SYNC MISMATCH
========================= */
void handle_sync_mismatch(int client_socket)
{
    printf("Chain mismatch detected. Re-requesting full height.\n");
    send(client_socket, "GET_HEIGHT\n", 11, 0);
}
