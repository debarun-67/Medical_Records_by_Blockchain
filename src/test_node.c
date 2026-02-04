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
   PRINT BLOCK DETAILS
========================================= */
void print_block(Block *block)
{
    printf("--------------------------------------------------\n");
    printf("[BLOCK] Index: %d\n", block->index);
    printf("[BLOCK] Timestamp: %ld\n", block->timestamp);
    printf("[BLOCK] Previous Hash: %s\n", block->previous_hash);
    printf("[BLOCK] Block Hash: %s\n", block->block_hash);
    printf("[BLOCK] Validator Port: %d\n", block->validator_port);
    printf("[BLOCK] Transactions: %d\n", block->transaction_count);

    for (int i = 0; i < block->transaction_count; i++)
    {
        printf("  [TX %d]\n", i + 1);
        printf("     Patient ID: %s\n", block->transactions[i].patient_id);
        printf("     Doctor ID: %s\n", block->transactions[i].doctor_id);
        printf("     Data Hash: %s\n", block->transactions[i].data_hash);
        printf("     File Path: %s\n", block->transactions[i].data_pointer);
        printf("     Timestamp: %ld\n", block->transactions[i].timestamp);
    }
    printf("--------------------------------------------------\n");
}

/* =========================================
   MAIN
========================================= */
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <own_port> [peer_ports...]\n", argv[0]);
        return 1;
    }

    int own_port = atoi(argv[1]);

    char chain_filename[128];
    snprintf(chain_filename, sizeof(chain_filename),
             "data/blockchain_%d.dat", own_port);

    set_blockchain_file(chain_filename);

    printf("[SYSTEM] Node started on port %d\n", own_port);

    Block last_block;

    if (!get_last_block(&last_block))
    {
        printf("[BLOCKCHAIN] No chain found. Creating genesis block...\n");

        Block genesis;
        memset(&genesis, 0, sizeof(Block));
        genesis.validator_port = own_port;
        create_genesis_block(&genesis, own_port);
        add_block(&genesis);

        printf("[BLOCKCHAIN] Genesis created.\n");
    }

    pthread_t server_thread;
    pthread_create(&server_thread, NULL, server_runner, &own_port);

    sleep(1);

    for (int i = 2; i < argc; i++)
    {
        int peer_port = atoi(argv[i]);
        if (peer_port != own_port)
            connect_to_peer("127.0.0.1", peer_port);
    }

    sleep(2);

    printf("[SYNC] Initiating chain synchronization...\n");
    initiate_chain_sync();

    /* =========================================
       INTERACTIVE LOOP
    ========================================= */
    while (1)
    {
        char input[256];

        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        input[strcspn(input, "\r\n")] = 0;

        /* ---------------- ADD ---------------- */
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
                printf("[ERROR] File not found.\n");
                continue;
            }

            fseek(fp, 0, SEEK_END);
            long filesize = ftell(fp);
            rewind(fp);

            char *file_buffer = malloc(filesize + 1);
            fread(file_buffer, 1, filesize, fp);
            file_buffer[filesize] = '\0';
            fclose(fp);

            char file_hash[HASH_SIZE];
            sha256(file_buffer, file_hash);
            free(file_buffer);

            if (transaction_hash_exists(file_hash))
            {
                printf("[CONSENSUS] Duplicate record detected.\n");
                continue;
            }

            Block last_block;
            get_last_block(&last_block);

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

            calculate_block_hash(&new_block);

            new_block.validator_port = own_port;

            char private_key_path[64];
            snprintf(private_key_path, sizeof(private_key_path),
                     "keys/%d_private.pem", own_port);

            if (!sign_data(new_block.block_hash,
                           private_key_path,
                           new_block.validator_signature))
            {
                printf("[CRYPTO] Signing failed.\n");
                continue;
            }

            printf("[CONSENSUS] Proposing block %d\n", new_block.index);
            propose_block(&new_block);
        }

        /* ---------------- HEIGHT ---------------- */
        else if (strcmp(input, "HEIGHT") == 0)
        {
            printf("[CHAIN] Height: %d\n", get_blockchain_height());
        }

        /* ---------------- LAST ---------------- */
        else if (strcmp(input, "LAST") == 0)
        {
            if (get_last_block(&last_block))
                print_block(&last_block);
        }

        /* ---------------- PRINT ---------------- */
        else if (strncmp(input, "PRINT ", 6) == 0)
        {
            int index = atoi(input + 6);
            Block block;

            if (get_block_by_index(index, &block))
                print_block(&block);
            else
                printf("[CHAIN] Block not found.\n");
        }

        /* ---------------- VERIFY ---------------- */
        else if (strcmp(input, "VERIFY") == 0)
        {
            if (verify_blockchain())
                printf("[VERIFY] Blockchain integrity: VALID\n");
            else
                printf("[VERIFY] Blockchain integrity: CORRUPTED\n");
        }

        /* ---------------- PEERS ---------------- */
        else if (strcmp(input, "PEERS") == 0)
        {
            printf("[NETWORK] Connected Peers: %d\n", get_peer_count());
        }

        /* ---------------- SYNC ---------------- */
        else if (strcmp(input, "SYNC") == 0)
        {
            printf("[SYNC] Manual sync triggered.\n");
            initiate_chain_sync();
        }

        /* ---------------- HASH ---------------- */
        else if (strncmp(input, "HASH ", 5) == 0)
        {
            char filename[256];
            sscanf(input + 5, "%255s", filename);

            char filepath[512];
            snprintf(filepath, sizeof(filepath),
                     "offchain/records/%s", filename);

            FILE *fp = fopen(filepath, "rb");
            if (!fp)
            {
                printf("[ERROR] File not found.\n");
                continue;
            }

            fseek(fp, 0, SEEK_END);
            long size = ftell(fp);
            rewind(fp);

            char *buffer = malloc(size + 1);
            fread(buffer, 1, size, fp);
            buffer[size] = '\0';
            fclose(fp);

            char hash[HASH_SIZE];
            sha256(buffer, hash);
            free(buffer);

            printf("[HASH] %s\n", hash);
        }

        /* ---------------- CHECKDUP ---------------- */
        else if (strncmp(input, "CHECKDUP ", 9) == 0)
        {
            char filename[256];
            sscanf(input + 9, "%255s", filename);

            char filepath[512];
            snprintf(filepath, sizeof(filepath),
                     "offchain/records/%s", filename);

            FILE *fp = fopen(filepath, "rb");
            if (!fp)
            {
                printf("[ERROR] File not found.\n");
                continue;
            }

            fseek(fp, 0, SEEK_END);
            long size = ftell(fp);
            rewind(fp);

            char *buffer = malloc(size + 1);
            fread(buffer, 1, size, fp);
            buffer[size] = '\0';
            fclose(fp);

            char hash[HASH_SIZE];
            sha256(buffer, hash);
            free(buffer);

            if (transaction_hash_exists(hash))
                printf("[CHAIN] Record already exists.\n");
            else
                printf("[CHAIN] Record not found in blockchain.\n");
        }

        /* ---------------- CHECKSIG ---------------- */
        else if (strncmp(input, "CHECKSIG ", 9) == 0)
        {
            int index = atoi(input + 9);
            Block block;

            if (!get_block_by_index(index, &block))
            {
                printf("[CHAIN] Block not found.\n");
                continue;
            }

            char public_key_path[64];
            snprintf(public_key_path, sizeof(public_key_path),
                     "keys/%d_public.pem", block.validator_port);

            if (verify_signature(block.block_hash,
                                 public_key_path,
                                 block.validator_signature))
                printf("[CRYPTO] Signature VALID.\n");
            else
                printf("[CRYPTO] Signature INVALID.\n");
        }

        /* ---------------- STATS ---------------- */
        else if (strcmp(input, "STATS") == 0)
        {
            printf("[STATS] Height: %d\n", get_blockchain_height());
            printf("[STATS] Connected Peers: %d\n", get_peer_count());
        }

        /* ---------------- HELP ---------------- */
        else if (strcmp(input, "HELP") == 0)
        {
            printf("Available Commands:\n");
            printf("ADD <file>\n");
            printf("HEIGHT\n");
            printf("LAST\n");
            printf("PRINT <index>\n");
            printf("VERIFY\n");
            printf("PEERS\n");
            printf("SYNC\n");
            printf("HASH <file>\n");
            printf("CHECKDUP <file>\n");
            printf("CHECKSIG <index>\n");
            printf("STATS\n");
            printf("HELP\n");
        }

        else
        {
            printf("[INFO] Unknown command. Type HELP.\n");
        }
    }

    return 0;
}
