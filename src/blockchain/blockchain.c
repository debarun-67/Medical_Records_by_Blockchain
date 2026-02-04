#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include "blockchain.h"
#include "../crypto/hash.h"
#include "../crypto/signature.h"

static char blockchain_file[128] = "data/blockchain.dat";

// mutex for thread safety
static pthread_mutex_t blockchain_lock = PTHREAD_MUTEX_INITIALIZER;

// set the blockchain file path
void set_blockchain_file(const char *filename)
{
    strncpy(blockchain_file, filename, sizeof(blockchain_file));
}

// create the first block (genesis)
void create_genesis_block(Block *block, int validator_port)
{
    memset(block, 0, sizeof(Block));

    block->index = 0;
    block->timestamp = 1737280140;
    block->validator_port = validator_port;

    strcpy(block->previous_hash, "0");

    block->transaction_count = 1;

    strcpy(block->transactions[0].patient_id, "GENESIS");
    strcpy(block->transactions[0].doctor_id, "NETWORK");

    const char *genesis_message =
        "The Fall of the star to the brink of an end from the loving pool";

    sha256(genesis_message,
           block->transactions[0].data_hash);

    strncpy(block->transactions[0].data_pointer,
            genesis_message,
            sizeof(block->transactions[0].data_pointer) - 1);

    block->transactions[0].timestamp = 1737280140;

    calculate_block_hash(block);

    char private_key_path[64];
    snprintf(private_key_path, sizeof(private_key_path),
             "keys/%d_private.pem", validator_port);

    if (!sign_data(block->block_hash,
                   private_key_path,
                   block->validator_signature))
    {
        printf("[CRYPTO] Genesis signing failed.\n");
        exit(1);
    }

    printf("[BLOCKCHAIN] Genesis block created (node %d)\n",
           validator_port);
}

// append a block securely
void add_block(Block *new_block)
{
    pthread_mutex_lock(&blockchain_lock);

    FILE *fp = fopen(blockchain_file, "ab");
    if (!fp)
    {
        pthread_mutex_unlock(&blockchain_lock);
        printf("[STORAGE] Failed to open blockchain file.\n");
        return;
    }

    fwrite(new_block, sizeof(Block), 1, fp);
    fflush(fp);
    fclose(fp);

    pthread_mutex_unlock(&blockchain_lock);
}

// retrieve the last block locally
int get_last_block(Block *last_block)
{
    pthread_mutex_lock(&blockchain_lock);

    FILE *fp = fopen(blockchain_file, "rb");
    if (!fp)
    {
        pthread_mutex_unlock(&blockchain_lock);
        return 0;
    }

    Block temp;
    int found = 0;

    while (fread(&temp, sizeof(Block), 1, fp) == 1)
    {
        *last_block = temp;
        found = 1;
    }

    fclose(fp);
    pthread_mutex_unlock(&blockchain_lock);

    return found;
}

// get latest block hash
int get_last_block_hash(char *output_hash)
{
    Block last_block;

    if (!get_last_block(&last_block))
        return 0;

    strcpy(output_hash, last_block.block_hash);
    return 1;
}

// validate the entire chain
int verify_blockchain()
{
    pthread_mutex_lock(&blockchain_lock);

    FILE *fp = fopen(blockchain_file, "rb");
    if (!fp)
    {
        pthread_mutex_unlock(&blockchain_lock);
        return 0;
    }

    Block prev, curr;

    if (fread(&prev, sizeof(Block), 1, fp) != 1)
    {
        fclose(fp);
        pthread_mutex_unlock(&blockchain_lock);
        return 0;
    }

    char stored_hash[HASH_SIZE];
    strcpy(stored_hash, prev.block_hash);

    calculate_block_hash(&prev);

    if (strcmp(stored_hash, prev.block_hash) != 0)
    {
        printf("[BLOCKCHAIN] Genesis hash validation failed.\n");
        fclose(fp);
        pthread_mutex_unlock(&blockchain_lock);
        return 0;
    }

    char public_key_path[64];
    snprintf(public_key_path, sizeof(public_key_path),
             "keys/%d_public.pem", prev.validator_port);

    if (!verify_signature(stored_hash,
                          public_key_path,
                          prev.validator_signature))
    {
        printf("[CRYPTO] Genesis signature validation failed.\n");
        fclose(fp);
        pthread_mutex_unlock(&blockchain_lock);
        return 0;
    }

    strcpy(stored_hash, prev.block_hash);

    while (fread(&curr, sizeof(Block), 1, fp) == 1)
    {
        if (strcmp(curr.previous_hash, stored_hash) != 0)
        {
            printf("[BLOCKCHAIN] Previous hash mismatch at block %d.\n",
                   curr.index);
            fclose(fp);
            pthread_mutex_unlock(&blockchain_lock);
            return 0;
        }

        char original_hash[HASH_SIZE];
        strcpy(original_hash, curr.block_hash);

        calculate_block_hash(&curr);

        if (strcmp(original_hash, curr.block_hash) != 0)
        {
            printf("[BLOCKCHAIN] Hash validation failed at block %d.\n",
                   curr.index);
            fclose(fp);
            pthread_mutex_unlock(&blockchain_lock);
            return 0;
        }

        snprintf(public_key_path, sizeof(public_key_path),
                 "keys/%d_public.pem", curr.validator_port);

        if (!verify_signature(original_hash,
                              public_key_path,
                              curr.validator_signature))
        {
            printf("[CRYPTO] Signature validation failed at block %d.\n",
                   curr.index);
            fclose(fp);
            pthread_mutex_unlock(&blockchain_lock);
            return 0;
        }

        strcpy(stored_hash, original_hash);
        prev = curr;
    }

    fclose(fp);
    pthread_mutex_unlock(&blockchain_lock);

    return 1;
}

// get chain length
int get_blockchain_height()
{
    pthread_mutex_lock(&blockchain_lock);

    FILE *fp = fopen(blockchain_file, "rb");
    if (!fp)
    {
        pthread_mutex_unlock(&blockchain_lock);
        return 0;
    }

    int count = 0;
    Block temp;

    while (fread(&temp, sizeof(Block), 1, fp) == 1)
        count++;

    fclose(fp);
    pthread_mutex_unlock(&blockchain_lock);

    return count;
}

// find block by index
int get_block_by_index(int index, Block *block)
{
    pthread_mutex_lock(&blockchain_lock);

    FILE *fp = fopen(blockchain_file, "rb");
    if (!fp)
    {
        pthread_mutex_unlock(&blockchain_lock);
        return 0;
    }

    Block temp;

    while (fread(&temp, sizeof(Block), 1, fp) == 1)
    {
        if (temp.index == index)
        {
            *block = temp;
            fclose(fp);
            pthread_mutex_unlock(&blockchain_lock);
            return 1;
        }
    }

    fclose(fp);
    pthread_mutex_unlock(&blockchain_lock);

    return 0;
}

// check if block exists
int block_exists_by_index(int index)
{
    pthread_mutex_lock(&blockchain_lock);

    FILE *fp = fopen(blockchain_file, "rb");
    if (!fp)
    {
        pthread_mutex_unlock(&blockchain_lock);
        return 0;
    }

    Block temp;

    while (fread(&temp, sizeof(Block), 1, fp) == 1)
    {
        if (temp.index == index)
        {
            fclose(fp);
            pthread_mutex_unlock(&blockchain_lock);
            return 1;
        }
    }

    fclose(fp);
    pthread_mutex_unlock(&blockchain_lock);

    return 0;
}

// checking for duplicate transactions
int transaction_hash_exists(const char *data_hash)
{
    pthread_mutex_lock(&blockchain_lock);

    FILE *fp = fopen(blockchain_file, "rb");
    if (!fp)
    {
        pthread_mutex_unlock(&blockchain_lock);
        return 0;
    }

    Block temp;

    while (fread(&temp, sizeof(Block), 1, fp) == 1)
    {
        for (int i = 0; i < temp.transaction_count; i++)
        {
            if (strcmp(temp.transactions[i].data_hash,
                       data_hash) == 0)
            {
                fclose(fp);
                pthread_mutex_unlock(&blockchain_lock);
                return 1;
            }
        }
    }

    fclose(fp);
    pthread_mutex_unlock(&blockchain_lock);
    return 0;
}
