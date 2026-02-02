#ifndef PROPOSAL_H
#define PROPOSAL_H

#include "../blockchain/block.h"

void propose_block(Block *block);
void register_vote(const char *vote);
void handle_commit(const char *serialized_block);

#endif
