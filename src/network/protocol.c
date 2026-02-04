#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

#include "node.h"
#include "protocol.h"
#include "proposal.h"
#include "../crypto/signature.h"
#include "serializer.h"
#include "sync.h"
#include "../blockchain/blockchain.h"

/* ===============================
   GLOBAL SYNC STATE
================================= */

static int sync_socket = -1;
static int sync_target_height = 0;
static int syncing = 0;

/* =========================================================
   PROTOCOL DISPATCH
========================================================= */

void protocol_dispatch(int client_socket, const char *message)
{
    char clean_message[BUFFER_SIZE];
    memset(clean_message, 0, sizeof(clean_message));

    strncpy(clean_message, message, BUFFER_SIZE - 1);

    /* Remove newline safely */
    size_t len = strlen(clean_message);
    while (len > 0 &&
          (clean_message[len - 1] == '\n' ||
           clean_message[len - 1] == '\r'))
    {
        clean_message[len - 1] = '\0';
        len--;
    }

    /* =========================================
       BLOCK VOTE
    ========================================== */
    if (strncmp(clean_message, "BLOCK_VOTE:", 11) == 0)
    {
        printf("[CONSENSUS] Vote received: %s\n", clean_message + 11);
        register_vote(clean_message);
        return;
    }

    /* =========================================
       COMMIT BLOCK
    ========================================== */
    if (strncmp(clean_message, "COMMIT_BLOCK:", 13) == 0)
    {
        printf("[CONSENSUS] Commit instruction received.\n");
        handle_commit(clean_message + 13);
        return;
    }

    /* =========================================
       CHAIN HEIGHT RESPONSE
    ========================================== */
    if (strncmp(clean_message, "CHAIN_HEIGHT:", 13) == 0)
    {
        int peer_height = atoi(clean_message + 13);
        int local_height = get_blockchain_height();

        if (peer_height > local_height && !syncing)
        {
            printf("[SYNC] Peer chain height %d > local %d. Initiating sync.\n",
                   peer_height, local_height);

            syncing = 1;
            sync_socket = client_socket;
            sync_target_height = peer_height;

            char request[64];
            snprintf(request, sizeof(request),
                     "GET_BLOCK:%d\n", local_height);

            send(sync_socket, request, strlen(request), 0);
        }

        return;
    }

    /* =========================================
       SYNC BLOCK
    ========================================== */
    if (strncmp(clean_message, "SYNC_BLOCK:", 11) == 0)
    {
        if (!syncing || client_socket != sync_socket)
            return;

        const char *serialized = clean_message + 11;

        Block incoming;
        memset(&incoming, 0, sizeof(Block));

        if (!deserialize_block(serialized, &incoming))
            return;

        int local_height = get_blockchain_height();

        if (incoming.index != local_height)
        {
            printf("[SYNC] Out-of-order block %d ignored.\n", incoming.index);
            return;
        }

        Block last_block;
        if (!get_last_block(&last_block))
            return;

        if (strcmp(incoming.previous_hash,
                   last_block.block_hash) != 0)
        {
            printf("[SYNC] Previous hash mismatch during sync.\n");
            syncing = 0;
            return;
        }

        printf("[SYNC] Appending block %d\n", incoming.index);

        add_block(&incoming);

        local_height++;

        if (local_height < sync_target_height)
        {
            char request[64];
            snprintf(request, sizeof(request),
                     "GET_BLOCK:%d\n", local_height);

            send(sync_socket, request, strlen(request), 0);
        }
        else
        {
            printf("[SYNC] Synchronization complete.\n");
            syncing = 0;
        }

        return;
    }

    /* =========================================
       REQUEST TYPES
    ========================================== */

    if (strcmp(clean_message, "GET_HEIGHT") == 0)
    {
        int height = get_blockchain_height();

        char response[64];
        snprintf(response, sizeof(response),
                 "CHAIN_HEIGHT:%d\n", height);

        send(client_socket, response, strlen(response), 0);
        return;
    }

    if (strncmp(clean_message, "GET_BLOCK:", 10) == 0)
    {
        int index = atoi(clean_message + 10);

        Block block;

        if (get_block_by_index(index, &block))
        {
            char buffer[SERIALIZED_BLOCK_SIZE];
            serialize_block(&block, buffer);

            char msg[SERIALIZED_BLOCK_SIZE + 32];
            snprintf(msg, sizeof(msg),
                     "SYNC_BLOCK:%s\n", buffer);

            send(client_socket, msg, strlen(msg), 0);
        }

        return;
    }

    /* =========================================
       PROPOSE BLOCK
    ========================================== */
    if (strncmp(clean_message, "PROPOSE_BLOCK:", 14) == 0)
    {
        Block incoming;
        memset(&incoming, 0, sizeof(Block));

        if (!deserialize_block(clean_message + 14, &incoming))
        {
            printf("[CONSENSUS] Block rejected: Deserialize failed.\n");
            send(client_socket, "BLOCK_VOTE:REJECT\n", 18, 0);
            return;
        }

        Block last_block;

        if (!get_last_block(&last_block))
        {
            printf("[CONSENSUS] Block rejected: No last block.\n");
            send(client_socket, "BLOCK_VOTE:REJECT\n", 18, 0);
            return;
        }

        if (incoming.index != last_block.index + 1)
        {
            printf("[CONSENSUS] Block %d rejected: Index mismatch.\n",
                   incoming.index);
            send(client_socket, "BLOCK_VOTE:REJECT\n", 18, 0);
            return;
        }

        if (strcmp(incoming.previous_hash,
                   last_block.block_hash) != 0)
        {
            printf("[CONSENSUS] Block %d rejected: Previous hash mismatch.\n",
                   incoming.index);
            send(client_socket, "BLOCK_VOTE:REJECT\n", 18, 0);
            return;
        }

        if (!verify_blockchain())
        {
            printf("[CONSENSUS] Block %d rejected: Local chain invalid.\n",
                   incoming.index);
            send(client_socket, "BLOCK_VOTE:REJECT\n", 18, 0);
            return;
        }

        printf("[CONSENSUS] Block %d approved.\n", incoming.index);
        send(client_socket, "BLOCK_VOTE:APPROVE\n", 19, 0);
        return;
    }

    /* Ignore unknown messages silently */
}
