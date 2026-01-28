#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include "block.h"

void create_genesis_block(Block *block);
void add_block(Block *new_block);
int get_last_block(Block *last_block);
int verify_blockchain();

#endif
