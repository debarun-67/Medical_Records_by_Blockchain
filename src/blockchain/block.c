#include <string.h>
#include <time.h>
#include "block.h"

/* Initialize a block with index and previous hash */
void init_block(Block *block, int index, const char *prev_hash) {
    block->index = index;
    block->timestamp = time(NULL);
    strcpy(block->previous_hash, prev_hash);
    block->transaction_count = 0;
}

/* Add a transaction to a block safely */
int add_transaction(Block *block, Transaction tx) {
    if (block->transaction_count >= MAX_TRANSACTIONS) {
        return 0; // Block full
    }

    block->transactions[block->transaction_count++] = tx;
    return 1;
}
