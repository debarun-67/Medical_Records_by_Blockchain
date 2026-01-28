#ifndef BLOCK_H
#define BLOCK_H

#include <time.h>

#define HASH_SIZE 65
#define MAX_TRANSACTIONS 5

typedef struct {
    char patient_id[32];        // pseudonymized
    char doctor_id[32];
    char data_hash[HASH_SIZE];  // hash of encrypted file
    char data_pointer[128];     // file:// or ipfs://
    time_t timestamp;
} Transaction;

typedef struct {
    int index;
    time_t timestamp;
    char previous_hash[HASH_SIZE];
    char block_hash[HASH_SIZE];
    char validator_signature[HASH_SIZE];
    Transaction transactions[MAX_TRANSACTIONS];
    int transaction_count;
} Block;

void init_block(Block *block, int index, const char *prev_hash);
int add_transaction(Block *block, Transaction tx);

#endif
