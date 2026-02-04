#ifndef SERIALIZER_H
#define SERIALIZER_H

#include "../blockchain/block.h"

#define SERIALIZED_BLOCK_SIZE 8192

void serialize_block(Block *block, char *buffer);
int deserialize_block(const char *buffer, Block *block);

#endif
