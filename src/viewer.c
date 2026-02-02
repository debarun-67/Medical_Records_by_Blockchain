#include <stdio.h>
#include <string.h>
#include <time.h>
#include "blockchain/block.h"

int main() {
    FILE *fp = fopen("data/blockchain_8001.dat", "rb");
    if (!fp) {
        printf("Blockchain file not found.\n");
        return 1;
    }

    Block block;
    printf("\n----- BLOCKCHAIN CONTENT -----\n");

    while (fread(&block, sizeof(Block), 1, fp)) {
        printf("\nBlock Index: %d\n", block.index);
        printf("Timestamp: %ld\n", block.timestamp);
        printf("Previous Hash: %s\n", block.previous_hash);
        printf("Block Hash: %s\n", block.block_hash);
        printf("Validator Signature: %s\n", block.validator_signature);
        printf("Transaction Count: %d\n", block.transaction_count);

        for (int i = 0; i < block.transaction_count; i++) {
            printf("  Transaction %d:\n", i + 1);
            printf("    Patient ID: %s\n", block.transactions[i].patient_id);
            printf("    Doctor ID: %s\n", block.transactions[i].doctor_id);
            printf("    Data Hash: %s\n", block.transactions[i].data_hash);
            printf("    Data Pointer: %s\n", block.transactions[i].data_pointer);
        }
    }

    fclose(fp);
    return 0;
}
