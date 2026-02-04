#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "serializer.h"

/* ---------------------------------
   Serialize Block
---------------------------------- */
void serialize_block(Block *block, char *buffer)
{
    buffer[0] = '\0';

    char line[4096];

    /*
      Header Format:
      index|timestamp|previous_hash|block_hash|validator_port|signature|tx_count~
    */
    snprintf(line, sizeof(line),
             "%d|%ld|%s|%s|%d|%s|%d~",
             block->index,
             block->timestamp,
             block->previous_hash,
             block->block_hash,
             block->validator_port,          // ✅ included
             block->validator_signature,
             block->transaction_count);

    strncat(buffer, line, SERIALIZED_BLOCK_SIZE - strlen(buffer) - 1);

    /* Transactions */
    for (int i = 0; i < block->transaction_count; i++)
    {
        snprintf(line, sizeof(line),
                 "TX|%s|%s|%s|%s|%ld~",
                 block->transactions[i].patient_id,
                 block->transactions[i].doctor_id,
                 block->transactions[i].data_hash,
                 block->transactions[i].data_pointer,
                 block->transactions[i].timestamp);

        strncat(buffer, line, SERIALIZED_BLOCK_SIZE - strlen(buffer) - 1);
    }

    strncat(buffer, "END_BLOCK~", SERIALIZED_BLOCK_SIZE - strlen(buffer) - 1);
}

/* ---------------------------------
   Deserialize Block
---------------------------------- */
int deserialize_block(const char *buffer, Block *block)
{
    memset(block, 0, sizeof(Block));

    char copy[SERIALIZED_BLOCK_SIZE];
    strncpy(copy, buffer, sizeof(copy) - 1);
    copy[sizeof(copy) - 1] = '\0';

    char *line = strtok(copy, "~");

    /* ---------- Parse Header ---------- */
    if (!line)
        return 0;

    /*
      Expected Header:
      index|timestamp|previous_hash|block_hash|validator_port|signature|tx_count
    */
    if (sscanf(line,
               "%d|%ld|%64[^|]|%64[^|]|%d|%512[^|]|%d",
               &block->index,
               &block->timestamp,
               block->previous_hash,
               block->block_hash,
               &block->validator_port,        // ✅ parsed
               block->validator_signature,
               &block->transaction_count) != 7)
        return 0;

    if (block->transaction_count < 0 ||
        block->transaction_count > MAX_TRANSACTIONS)
        return 0;

    /* ---------- Parse Transactions ---------- */
    int tx_index = 0;

    while ((line = strtok(NULL, "~")) != NULL)
    {
        if (strcmp(line, "END_BLOCK") == 0)
            break;

        if (strncmp(line, "TX|", 3) == 0)
        {
            if (tx_index >= MAX_TRANSACTIONS)
                return 0;

            if (sscanf(line,
                       "TX|%31[^|]|%31[^|]|%64[^|]|%127[^|]|%ld",
                       block->transactions[tx_index].patient_id,
                       block->transactions[tx_index].doctor_id,
                       block->transactions[tx_index].data_hash,
                       block->transactions[tx_index].data_pointer,
                       &block->transactions[tx_index].timestamp) != 5)
                return 0;

            tx_index++;
        }
    }

    /* Final sanity check */
    if (tx_index != block->transaction_count)
        return 0;

    return 1;
}
