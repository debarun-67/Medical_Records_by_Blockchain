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

    printf("Dispatching message: %s\n", clean_message);

    /* =========================================
       BLOCK VOTE
    ========================================== */

    if (strncmp(clean_message, "BLOCK_VOTE:", 11) == 0)
    {
        register_vote(clean_message);
        return;
    }

    /* =========================================
       COMMIT BLOCK
    ========================================== */

    if (strncmp(clean_message, "COMMIT_BLOCK:", 13) == 0)
    {
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
            printf("Selecting peer for sync...\n");

            syncing = 1;
            sync_socket = client_socket;
            sync_target_height = peer_height;

            /* Request first missing block */
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
            return;  // ignore blocks from other peers

        const char *serialized = clean_message + 11;

        Block incoming;
        memset(&incoming, 0, sizeof(Block));

        if (!deserialize_block(serialized, &incoming))
            return;

        int local_height = get_blockchain_height();

        /* Strict sequential rule */
        if (incoming.index != local_height)
        {
            printf("Out of order block. Ignoring.\n");
            return;
        }

        Block last_block;

        if (!get_last_block(&last_block))
            return;

        if (strcmp(incoming.previous_hash,
                   last_block.block_hash) != 0)
        {
            printf("Sync previous hash mismatch.\n");
            syncing = 0;
            return;
        }

        printf("Synchronizing block %d\n", incoming.index);

        add_block(&incoming);

        local_height++;

        /* Continue sync if more blocks needed */
        if (local_height < sync_target_height)
        {
            char request[64];
            snprintf(request, sizeof(request),
                     "GET_BLOCK:%d\n", local_height);

            send(sync_socket, request, strlen(request), 0);
        }
        else
        {
            printf("Sync complete.\n");
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
    if (syncing)
    {
        send(client_socket, "BLOCK_VOTE:REJECT\n", 18, 0);
        return;
    }

    Block incoming;
    memset(&incoming, 0, sizeof(Block));

    if (!deserialize_block(clean_message + 14, &incoming))
    {
        send(client_socket, "BLOCK_VOTE:REJECT\n", 18, 0);
        return;
    }

    /* =====================================================
       1️⃣ Reject duplicate medical record
    ===================================================== */
    if (transaction_hash_exists(incoming.transactions[0].data_hash))
    {
        printf("Duplicate medical record detected.\n");
        send(client_socket, "BLOCK_VOTE:REJECT\n", 18, 0);
        return;
    }

    /* =====================================================
       2️⃣ Recompute block hash and verify integrity
    ===================================================== */
    char original_hash[HASH_SIZE];
    strcpy(original_hash, incoming.block_hash);

    calculate_block_hash(&incoming);

    if (strcmp(original_hash, incoming.block_hash) != 0)
    {
        printf("Block hash tampered.\n");
        send(client_socket, "BLOCK_VOTE:REJECT\n", 18, 0);
        return;
    }

    /* =====================================================
       3️⃣ Verify RSA Signature
    ===================================================== */
    char public_key_path[64];
    snprintf(public_key_path, sizeof(public_key_path),
             "keys/%d_public.pem",
             incoming.validator_port);

    if (!verify_signature(original_hash,
                          public_key_path,
                          incoming.validator_signature))
    {
        printf("Signature verification failed.\n");
        send(client_socket, "BLOCK_VOTE:REJECT\n", 18, 0);
        return;
    }

    /* =====================================================
       4️⃣ Check chain extension
    ===================================================== */
    Block last_block;

    if (!get_last_block(&last_block))
    {
        send(client_socket, "BLOCK_VOTE:REJECT\n", 18, 0);
        return;
    }

    if (incoming.index != last_block.index + 1)
    {
        printf("Index mismatch.\n");
        send(client_socket, "BLOCK_VOTE:REJECT\n", 18, 0);
        return;
    }

    if (strcmp(incoming.previous_hash,
               last_block.block_hash) != 0)
    {
        printf("Previous hash mismatch.\n");
        send(client_socket, "BLOCK_VOTE:REJECT\n", 18, 0);
        return;
    }

    /* =====================================================
       5️⃣ Verify entire local chain integrity
    ===================================================== */
    if (!verify_blockchain())
    {
        printf("Local chain corrupted.\n");
        send(client_socket, "BLOCK_VOTE:REJECT\n", 18, 0);
        return;
    }

    /* =====================================================
       ✅ Everything valid
    ===================================================== */
    send(client_socket, "BLOCK_VOTE:APPROVE\n", 19, 0);
    return;
}

    /* Ignore others */
}
