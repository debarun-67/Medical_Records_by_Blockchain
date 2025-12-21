#include <stdio.h>
#include <string.h>
#include "../blockchain/blockchain.h"
#include "../crypto/hash.h"

int main() {
    Block genesis;
    create_genesis_block(&genesis);
    add_block(&genesis);

    printf("Genesis block created.\n");

    Block block;
    block.index = 1;
    block.timestamp = time(NULL);
    strcpy(block.previous_hash, genesis.block_hash);

    Transaction tx;
    strcpy(tx.patient_id, "PATIENT123");
    strcpy(tx.doctor_id, "DOCTOR01");
    strcpy(tx.data_pointer, "file://offchain/storage/record1.enc");
    sha256("encrypted_record_content", tx.data_hash);
    tx.timestamp = time(NULL);

    block.transactions[0] = tx;
    block.transaction_count = 1;

    char data_to_hash[256];
    snprintf(data_to_hash, sizeof(data_to_hash), "%d%s", block.index, tx.data_hash);
    sha256(data_to_hash, block.block_hash);

    sign_data(block.block_hash, "hospital_private_key", block.validator_signature);
    add_block(&block);

    printf("Block added.\n");

    if (verify_blockchain())
        printf("Blockchain verified successfully.\n");
    else
        printf("Blockchain verification failed.\n");

    return 0;
}
