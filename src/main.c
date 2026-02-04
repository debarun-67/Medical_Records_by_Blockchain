#include <stdio.h>
#include <string.h>
#include <time.h>

#include "blockchain/blockchain.h"
#include "blockchain/block.h"
#include "crypto/hash.h"
#include "crypto/signature.h"

#define OFFCHAIN_DIR "offchain/records/"

int hash_file(const char *filename, char *output_hash) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("ERROR: Cannot open file %s\n", filename);
        return 0;
    }

    char line[256];
    char first_line[256] = "";
    char last_line[256] = "";
    int line_count = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (line_count == 0)
            strcpy(first_line, line);

        strcpy(last_line, line);
        line_count++;
    }

    fclose(fp);


    char fingerprint[256];
    snprintf(
        fingerprint,
        sizeof(fingerprint),
        "%s|%s|%d",
        first_line,
        last_line,
        line_count
    );

    sha256(fingerprint, output_hash);
    return 1;
}

int record_exists(const char *data_hash) {
    FILE *fp = fopen("data/blockchain.dat", "rb");
    if (!fp) return 0;

    Block block;
    while (fread(&block, sizeof(Block), 1, fp) == 1) {
        for (int i = 0; i < block.transaction_count; i++) {
            if (strcmp(block.transactions[i].data_hash, data_hash) == 0) {
                fclose(fp);
                return 1;
            }
        }
    }

    fclose(fp);
    return 0;
}

// main entry point
int main() {
    printf("Blockchain starting...\n");

    Block last_block;
    int has_chain = get_last_block(&last_block);

    // handle genesis block if chain is empty
    if (!has_chain) {
        printf("No blockchain found. Creating genesis block...\n");
        create_genesis_block(&last_block);
        add_block(&last_block);

        // reload to ensure we have the latest state
        get_last_block(&last_block);
    }

    Transaction tx;
    char record_name[128];

    // setting up demo metadata
    strcpy(tx.patient_id, "HOSP-IND-2025-001124");
    strcpy(tx.doctor_id, "DR-KOL-GYN-118");

    printf("Enter encrypted record file name (e.g., record1.enc): ");
    scanf("%s", record_name);

    snprintf(tx.data_pointer,
             sizeof(tx.data_pointer),
             "%s%s",
             OFFCHAIN_DIR,
             record_name);

    // hash the file content
    if (!hash_file(tx.data_pointer, tx.data_hash)) {
        return 1;
    }

    // avoid duplicates
    if (record_exists(tx.data_hash)) {
        printf("ERROR: This medical record already exists in the blockchain.\n");
        return 0;
    }

    tx.timestamp = time(NULL);

    // create new block
    Block block;
    init_block(&block, last_block.index + 1, last_block.block_hash);
    add_transaction(&block, tx);

    // calculate hash and sign the block
    sha256(tx.data_hash, block.block_hash);
    sign_data(block.block_hash,
              "hospital_private_key",
              block.validator_signature);

    add_block(&block);
    printf("Medical record block added.\n");

    // integrity check
    if (verify_blockchain()) {
        printf("Blockchain verified successfully.\n");
    } else {
        printf("Blockchain verification failed.\n");
    }

    return 0;
}
