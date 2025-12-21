#include <stdio.h>
#include <string.h>
#include <time.h>

/* Project headers */
#include "blockchain/blockchain.h"
#include "crypto/hash.h"
#include "crypto/signature.h"

int main() {
    printf("Blockchain starting...\n");

    /* -------------------------------
       STEP 1: Create Genesis Block
       ------------------------------- */
    Block genesis;
    create_genesis_block(&genesis);
    add_block(&genesis);
    printf("Genesis block created.\n");

    /* -------------------------------
       STEP 2: Create Medical Record Block
       ------------------------------- */
    Block block;
    block.index = 1;
    block.timestamp = time(NULL);
    strcpy(block.previous_hash, genesis.block_hash);
    block.transaction_count = 1;

    /* Create a medical record transaction */
    Transaction tx;
    strcpy(tx.patient_id, "PATIENT123");                 // pseudonymized
    strcpy(tx.doctor_id, "DOCTOR01");
    strcpy(tx.data_pointer, "file://offchain/storage/record1.enc");
    sha256("encrypted_medical_record_sample", tx.data_hash);
    tx.timestamp = time(NULL);

    block.transactions[0] = tx;

    /* -------------------------------
       STEP 3: Hash and Sign Block
       ------------------------------- */
    char data_to_hash[256];
    snprintf(
        data_to_hash,
        sizeof(data_to_hash),
        "%d%s%s",
        block.index,
        block.previous_hash,
        tx.data_hash
    );

    sha256(data_to_hash, block.block_hash);

    /* Proof of Authority: validator signs the block */
    sign_data(
        block.block_hash,
        "hospital_private_key",
        block.validator_signature
    );

    add_block(&block);
    printf("Medical record block added.\n");

    /* -------------------------------
       STEP 4: Verify Blockchain
       ------------------------------- */
    if (verify_blockchain()) {
        printf("Blockchain verified successfully.\n");
    } else {
        printf("Blockchain verification failed.\n");
    }

    return 0;
}
