#include <stdio.h>
#include <string.h>
#include <time.h>

#include "block.h"
#include "../crypto/hash.h"

// initialize a new block
void init_block(Block *block, int index, const char *prev_hash)
{
    block->index = index;
    block->timestamp = time(NULL);
    strcpy(block->previous_hash, prev_hash);
    block->transaction_count = 0;

    memset(block->block_hash, 0, HASH_SIZE);
    memset(block->validator_signature, 0, HASH_SIZE);
}

// add a transaction to the block
int add_transaction(Block *block, Transaction tx)
{
    if (block->transaction_count >= MAX_TRANSACTIONS)
        return 0;

    block->transactions[block->transaction_count++] = tx;
    return 1;
}

// generate hash for the block
void calculate_block_hash(Block *block)
{
    char buffer[2048];
    buffer[0] = '\0';

    char temp[256];

    // block metadata
    snprintf(temp, sizeof(temp),
             "%d%ld%s%d",
             block->index,
             block->timestamp,
             block->previous_hash,
             block->transaction_count);

    strcat(buffer, temp);

    // transaction data
    for (int i = 0; i < block->transaction_count; i++)
    {
        snprintf(temp, sizeof(temp),
                 "%s%s%s%s%ld",
                 block->transactions[i].patient_id,
                 block->transactions[i].doctor_id,
                 block->transactions[i].data_hash,
                 block->transactions[i].data_pointer,
                 block->transactions[i].timestamp);

        strcat(buffer, temp);
    }

    sha256(buffer, block->block_hash);
}


