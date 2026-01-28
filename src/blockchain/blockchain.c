#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "blockchain.h"
#include "../crypto/hash.h"
#include "../crypto/signature.h"

#define BLOCKCHAIN_FILE "data/blockchain.dat"
#define VALIDATOR_PRIVATE_KEY "hospital_private_key"
#define VALIDATOR_PUBLIC_KEY  "hospital_private_key"

/* ---------------------------------
   Create Genesis Block
---------------------------------- */
void create_genesis_block(Block *block) {
    block->index = 0;
    block->timestamp = time(NULL);
    strcpy(block->previous_hash, "0");
    block->transaction_count = 0;

    sha256("GENESIS", block->block_hash);
    sign_data(block->block_hash, VALIDATOR_PRIVATE_KEY, block->validator_signature);
}

/* ---------------------------------
   Add Block Safely
---------------------------------- */
void add_block(Block *new_block) {
    FILE *fp = fopen(BLOCKCHAIN_FILE, "ab");
    if (!fp) {
        printf("ERROR: Cannot open blockchain.dat for writing\n");
        return;
    }

    size_t written = fwrite(new_block, sizeof(Block), 1, fp);
    fclose(fp);

    if (written != 1) {
        printf("ERROR: Failed to write block to blockchain file\n");
    }
}

/* ---------------------------------
   Get Last Block (for appending)
---------------------------------- */
int get_last_block(Block *last_block) {
    FILE *fp = fopen(BLOCKCHAIN_FILE, "rb");
    if (!fp) return 0;

    Block temp;
    int found = 0;

    while (fread(&temp, sizeof(Block), 1, fp) == 1) {
        *last_block = temp;
        found = 1;
    }

    fclose(fp);
    return found;
}

/* ---------------------------------
   Verify Blockchain Integrity
---------------------------------- */
int verify_blockchain() {
    FILE *fp = fopen(BLOCKCHAIN_FILE, "rb");
    if (!fp) {
        printf("ERROR: Blockchain file not found.\n");
        return 0;
    }

    Block prev, curr;

    /* Read genesis block */
    if (fread(&prev, sizeof(Block), 1, fp) != 1) {
        fclose(fp);
        printf("ERROR: Failed to read genesis block.\n");
        return 0;
    }

    /* Verify genesis block signature */
    if (!verify_signature(prev.block_hash,
                          VALIDATOR_PUBLIC_KEY,
                          prev.validator_signature)) {
        fclose(fp);
        printf("ERROR: Invalid genesis block signature.\n");
        return 0;
    }

    /* Verify rest of the chain */
    while (fread(&curr, sizeof(Block), 1, fp) == 1) {

        /* Verify hash linkage */
        if (strcmp(curr.previous_hash, prev.block_hash) != 0) {
            fclose(fp);
            printf("ERROR: Hash mismatch at block %d\n", curr.index);
            return 0;
        }

        /* Verify validator signature */
        if (!verify_signature(curr.block_hash,
                              VALIDATOR_PUBLIC_KEY,
                              curr.validator_signature)) {
            fclose(fp);
            printf("ERROR: Invalid signature at block %d\n", curr.index);
            return 0;
        }

        prev = curr;
    }

    fclose(fp);
    return 1;
}
