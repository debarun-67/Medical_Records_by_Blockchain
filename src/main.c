#include <stdio.h>
#include <string.h>
#include <time.h>

/* Project headers */
#include "blockchain/blockchain.h"
#include "blockchain/block.h"
#include "crypto/hash.h"
#include "crypto/signature.h"

int main() {
    printf("Blockchain starting...\n");

    Block last_block;
    int has_chain = get_last_block(&last_block);

    /* --------------------------------
       STEP 1: Initialize Blockchain
    --------------------------------- */
    if (!has_chain) {
        printf("No blockchain found. Creating genesis block...\n");
        create_genesis_block(&last_block);
        add_block(&last_block);
    }

    /* --------------------------------
       STEP 2: Create New Medical Block
    --------------------------------- */
    Block block;
    init_block(&block, last_block.index + 1, last_block.block_hash);

    /* Create medical record transaction */
    Transaction tx;
    strcpy(tx.patient_id, "PATIENT123");                 
    strcpy(tx.doctor_id, "DOCTOR01");
    strcpy(tx.data_pointer, "file://offchain/storage_sim/record1.enc");

    /* Hash of off-chain medical record (safe small input) */
    sha256("encrypted_medical_record_sample", tx.data_hash);

    tx.timestamp = time(NULL);

    add_transaction(&block, tx);

    /* --------------------------------
       STEP 3: Hash and Sign Block
    --------------------------------- */
    sha256(tx.data_hash, block.block_hash);

    sign_data(
        block.block_hash,
        "hospital_private_key",
        block.validator_signature
    );

    add_block(&block);
    printf("Medical record block added.\n");

    /* --------------------------------
       STEP 4: Verify Blockchain
    --------------------------------- */
    int result = verify_blockchain();

    if (result == 1) {
        printf("Blockchain verified successfully.\n");
    } else {
        printf("Blockchain verification failed.\n");
    }

    return 0;
}
