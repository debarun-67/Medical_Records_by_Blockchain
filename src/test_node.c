#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

#include "network/node.h"
#include "network/serializer.h"
#include "network/proposal.h"
#include "network/sync.h"

#include "blockchain/block.h"
#include "blockchain/blockchain.h"

#include "crypto/hash.h"
#include "crypto/signature.h"

/* =========================================
   SERVER THREAD
========================================= */
void *server_runner(void *arg)
{
    int port = *(int *)arg;
    start_server(port);
    return NULL;
}

/* =========================================
   MAIN
========================================= */
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <own_port> [peer_port1] [peer_port2] ...\n", argv[0]);
        return 1;
    }

    int own_port = atoi(argv[1]);

    /* =====================================
       1️⃣  SET PER-NODE BLOCKCHAIN FILE
    ===================================== */
    char chain_filename[128];
    snprintf(chain_filename, sizeof(chain_filename),
             "data/blockchain_%d.dat", own_port);

    set_blockchain_file(chain_filename);

    /* =====================================
       2️⃣  CREATE GENESIS IF NEEDED
    ===================================== */
    Block last_block;

    if (!get_last_block(&last_block))
    {
        printf("No blockchain found. Creating genesis block...\n");

        Block genesis;
        memset(&genesis, 0, sizeof(Block));
        genesis.validator_port = own_port;
        create_genesis_block(&genesis, own_port);
        add_block(&genesis);
    }

    /* =====================================
       3️⃣  START SERVER
    ===================================== */
    pthread_t server_thread;
    pthread_create(&server_thread, NULL, server_runner, &own_port);

    sleep(1);

    /* =====================================
       4️⃣  CONNECT TO PEERS
    ===================================== */
    for (int i = 2; i < argc; i++)
    {
        int peer_port = atoi(argv[i]);

        if (peer_port == own_port)
            continue;

        connect_to_peer("127.0.0.1", peer_port);
    }

    sleep(2);

    /* =====================================
       5️⃣  INITIATE SYNC
    ===================================== */
    initiate_chain_sync();

    /* =====================================
       6️⃣  INTERACTIVE LOOP
    ===================================== */
    while (1)
    {
        char input[256];

        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        input[strcspn(input, "\r\n")] = 0;

        /* =====================================================
           ADD <filename>
        ===================================================== */
        if (strncmp(input, "ADD ", 4) == 0)
        {
            char record_filename[256];
            sscanf(input + 4, "%255s", record_filename);

            char filepath[512];
            snprintf(filepath, sizeof(filepath),
                     "offchain/records/%s", record_filename);

            FILE *fp = fopen(filepath, "rb");
            if (!fp)
            {
                printf("ERROR: File not found: %s\n", filepath);
                continue;
            }

            /* Read file safely */
            fseek(fp, 0, SEEK_END);
            long filesize = ftell(fp);
            rewind(fp);

            if (filesize <= 0)
            {
                printf("ERROR: Empty file.\n");
                fclose(fp);
                continue;
            }

            char *file_buffer = malloc(filesize + 1);
            if (!file_buffer)
            {
                printf("ERROR: Memory allocation failed.\n");
                fclose(fp);
                continue;
            }

            fread(file_buffer, 1, filesize, fp);
            file_buffer[filesize] = '\0';
            fclose(fp);

            /* Compute SHA256 of file */
            char file_hash[HASH_SIZE];
            sha256(file_buffer, file_hash);
            free(file_buffer);

            /* Prevent duplicate medical record */
            if (transaction_hash_exists(file_hash))
            {
                printf("Duplicate record detected. Block not proposed.\n");
                continue;
            }

            Block last_block;
            if (!get_last_block(&last_block))
            {
                printf("ERROR: No genesis block found.\n");
                continue;
            }

            /* Create new block */
            Block new_block;
            memset(&new_block, 0, sizeof(Block));

            init_block(&new_block,
                       last_block.index + 1,
                       last_block.block_hash);

            new_block.transaction_count = 1;

            strcpy(new_block.transactions[0].patient_id, "PATIENT_FROM_FILE");
            strcpy(new_block.transactions[0].doctor_id, "DOCTOR_FROM_FILE");

            strcpy(new_block.transactions[0].data_hash, file_hash);
            strcpy(new_block.transactions[0].data_pointer, filepath);

            new_block.transactions[0].timestamp = time(NULL);

            /* Compute block hash */
            calculate_block_hash(&new_block);

            /* Assign validator identity */
            new_block.validator_port = own_port;

            /* Sign block using RSA private key */
            char private_key_path[64];
            snprintf(private_key_path, sizeof(private_key_path),
                     "keys/%d_private.pem", own_port);

            if (!sign_data(new_block.block_hash,
                           private_key_path,
                           new_block.validator_signature))
            {
                printf("ERROR: Signing failed.\n");
                continue;
            }

            printf("Proposing block %d for file %s\n",
                   new_block.index, record_filename);

            propose_block(&new_block);
        }
        else
        {
            broadcast_message(input);
        }
    }

    return 0;
}
