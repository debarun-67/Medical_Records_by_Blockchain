#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "serializer.h"

void serialize_block(Block *block, char *buffer)
{
    buffer[0] = '\0';

    char line[512];

    /* Header */
    snprintf(line, sizeof(line),
             "%d|%ld|%s|%s|%s|%d~",
             block->index,
             block->timestamp,
             block->previous_hash,
             block->block_hash,
             block->validator_signature,
             block->transaction_count);

    strcat(buffer, line);

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

        strcat(buffer, line);
    }

    strcat(buffer, "END_BLOCK~");
}

int deserialize_block(const char *buffer, Block *block)
{
    memset(block, 0, sizeof(Block));

    char copy[SERIALIZED_BLOCK_SIZE];
    strncpy(copy, buffer, sizeof(copy) - 1);
    copy[sizeof(copy) - 1] = '\0';

    char *line = strtok(copy, "~");

    /* Parse header */
    if (!line)
        return 0;

    if (sscanf(line, "%d|%ld|%64[^|]|%64[^|]|%64[^|]|%d",
               &block->index,
               &block->timestamp,
               block->previous_hash,
               block->block_hash,
               block->validator_signature,
               &block->transaction_count) != 6)
        return 0;

    int tx_index = 0;

    while ((line = strtok(NULL, "\n")) != NULL)
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

    return 1;
}
