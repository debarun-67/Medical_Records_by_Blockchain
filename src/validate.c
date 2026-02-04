#include <stdio.h>
#include <string.h>

#include "blockchain/block.h"
#include "crypto/hash.h"

#define BLOCKCHAIN_FILE "data/blockchain.dat"
#define OFFCHAIN_DIR "offchain/records/"

// hash file matching blockchain method
int hash_file(const char *filename, char *output_hash) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        printf("ERROR: Cannot open file %s\n", filename);
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fclose(fp);

    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s:%ld", filename, size);
    sha256(buffer, output_hash);

    return 1;
}

int main() {
    char record_name[128];
    char record_path[256];
    char computed_hash[65];

    printf("Enter encrypted record file name to validate: ");
    scanf("%127s", record_name);

    snprintf(record_path, sizeof(record_path),
             "%s%s", OFFCHAIN_DIR, record_name);

    if (!hash_file(record_path, computed_hash)) {
        return 1;
    }

    FILE *fp = fopen(BLOCKCHAIN_FILE, "rb");
    if (!fp) {
        printf("ERROR: Blockchain file not found.\n");
        return 1;
    }

    Block block;
    int found = 0;

    while (fread(&block, sizeof(Block), 1, fp) == 1) {
        for (int i = 0; i < block.transaction_count; i++) {
            if (strcmp(block.transactions[i].data_pointer, record_path) == 0) {
                found = 1;

                printf("\n--- RECORD VALIDATION RESULT ---\n");
                printf("Stored Hash   : %s\n", block.transactions[i].data_hash);
                printf("Computed Hash : %s\n", computed_hash);

                if (strcmp(block.transactions[i].data_hash, computed_hash) == 0) {
                    printf("STATUS: Record is NOT altered.\n");
                } else {
                    printf("STATUS: Record HAS BEEN altered!\n");
                }

                fclose(fp);
                return 0;
            }
        }
    }

    fclose(fp);

    if (!found) {
        printf("ERROR: Record not found in blockchain.\n");
    }

    return 0;
}
