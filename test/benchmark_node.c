#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

#include "../src/network/node.h"
#include "../src/network/proposal.h"
#include "../src/network/sync.h"

#include "../src/blockchain/block.h"
#include "../src/blockchain/blockchain.h"

#include "../src/crypto/hash.h"
#include "../src/crypto/signature.h"

// server thread
void *server_runner(void *arg)
{
    int port = *(int *)arg;
    start_server(port);
    return NULL;
}

// main benchmark loop
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Usage: %s <own_port> <block_count> [peer_ports...]\n", argv[0]);
        return 1;
    }

    int own_port = atoi(argv[1]);
    int block_count = atoi(argv[2]);

    printf("\n========== BENCHMARK MODE ==========\n");
    printf("Node Port: %d\n", own_port);
    printf("Blocks to Commit: %d\n", block_count);
    printf("====================================\n");

    // set blockchain file path
    char chain_filename[128];
    snprintf(chain_filename, sizeof(chain_filename),
             "data/blockchain_%d.dat", own_port);

    set_blockchain_file(chain_filename);

    // initialize genesis block
    Block last_block;
    if (!get_last_block(&last_block))
    {
        Block genesis;
        memset(&genesis, 0, sizeof(Block));
        genesis.validator_port = own_port;
        create_genesis_block(&genesis, own_port);
        add_block(&genesis);
    }

    // launch network server
    pthread_t server_thread;
    pthread_create(&server_thread, NULL, server_runner, &own_port);

    sleep(1);

    // connect to known peers
    for (int i = 3; i < argc; i++)
    {
        int peer_port = atoi(argv[i]);
        if (peer_port != own_port)
            connect_to_peer("127.0.0.1", peer_port);
    }

    sleep(2);

    initiate_chain_sync();
    sleep(2);

    // start benchmarking

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    double total_commit_time = 0.0;

    for (int i = 0; i < block_count; i++)
    {
        if (!get_last_block(&last_block))
            continue;

        int expected_index = last_block.index + 1;

        Block new_block;
        memset(&new_block, 0, sizeof(Block));

        init_block(&new_block,
                   expected_index,
                   last_block.block_hash);

        new_block.transaction_count = 1;

        char dummy_data[128];
        snprintf(dummy_data, sizeof(dummy_data),
                 "BENCH_DATA_%d_%ld", i, time(NULL));

        char hash[HASH_SIZE];
        sha256(dummy_data, hash);

        strcpy(new_block.transactions[0].patient_id, "BENCH_PATIENT");
        strcpy(new_block.transactions[0].doctor_id, "BENCH_DOCTOR");
        strcpy(new_block.transactions[0].data_hash, hash);
        strcpy(new_block.transactions[0].data_pointer, "BENCH_DATA");

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
            printf("Signing failed.\n");
            continue;
        }

        struct timespec block_start, block_end;
        clock_gettime(CLOCK_MONOTONIC, &block_start);

        propose_block(&new_block);

        // wait for block finalization
        while (1)
        {
            Block check_block;
            if (get_last_block(&check_block))
            {
                if (check_block.index >= expected_index)
                    break;
            }
            usleep(1000);
        }

        clock_gettime(CLOCK_MONOTONIC, &block_end);

        double commit_time =
            (block_end.tv_sec - block_start.tv_sec) +
            (block_end.tv_nsec - block_start.tv_nsec) / 1e9;

        total_commit_time += commit_time;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double total_time =
        (end.tv_sec - start.tv_sec) +
        (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("\n========== BENCHMARK RESULTS ==========\n");
    printf("Blocks Successfully Committed: %d\n", block_count);
    printf("Total Benchmark Time: %.4f seconds\n", total_time);
    printf("Average Commit Latency: %.6f seconds\n",
           total_commit_time / block_count);
    printf("True Consensus Throughput: %.2f blocks/sec\n",
           block_count / total_time);
    printf("=======================================\n");

    return 0;
}
