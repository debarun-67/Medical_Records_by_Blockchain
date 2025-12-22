#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "blockchain.h"
#include "../crypto/hash.h"
#include "../crypto/signature.h"

#define BLOCKCHAIN_FILE "data/blockchain.dat"
#define VALIDATOR_PRIVATE_KEY "hospital_private_key"
#define VALIDATOR_PUBLIC_KEY  "hospital_private_key"

void create_genesis_block(Block *block) {
    block->index = 0;
    block->timestamp = time(NULL);
    strcpy(block->previous_hash, "0");
    block->transaction_count = 0;

    char content[] = "GENESIS";
    sha256(content, block->block_hash);
    sign_data(block->block_hash, VALIDATOR_PRIVATE_KEY, block->validator_signature);
}

void add_block(Block *new_block) {
    FILE *fp = fopen(BLOCKCHAIN_FILE, "ab");
    fwrite(new_block, sizeof(Block), 1, fp);
    fclose(fp);
}

int verify_blockchain() {
    FILE *fp = fopen("data/blockchain.dat", "rb");
    if (!fp) {
        printf("ERROR: Blockchain file not found.\n");
        return 0;
    }

    Block prev, curr;

    /* Read genesis block */
    if (fread(&prev, sizeof(Block), 1, fp) != 1) {
        fclose(fp);
        return 0;
    }

    /* Verify genesis block signature only */
    if (!verify_signature(prev.block_hash,
                          "hospital_private_key",
                          prev.validator_signature)) {
        fclose(fp);
        return 0;
    }

    /* Verify remaining blocks */
    while (fread(&curr, sizeof(Block), 1, fp) == 1) {

        /* Check hash linkage */
        if (strcmp(curr.previous_hash, prev.block_hash) != 0) {
            fclose(fp);
            return 0;
        }

        /* Verify validator signature */
        if (!verify_signature(curr.block_hash,
                              "hospital_private_key",
                              curr.validator_signature)) {
            fclose(fp);
            return 0;
        }

        prev = curr;
    }

    fclose(fp);
    return 1;
}

