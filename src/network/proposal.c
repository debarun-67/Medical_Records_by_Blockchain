#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "proposal.h"
#include "serializer.h"
#include "node.h"
#include "../blockchain/blockchain.h"

/* -------------------------
   Proposal State
-------------------------- */

static Block current_proposal;
static int proposal_active = 0;
static int approve_votes = 0;
static pthread_mutex_t vote_lock = PTHREAD_MUTEX_INITIALIZER;


/* -------------------------
   Propose Block
-------------------------- */
void propose_block(Block *block)
{
    pthread_mutex_lock(&vote_lock);

    current_proposal = *block;
    proposal_active = 1;

    /* Count self vote */
    approve_votes = 1;

    pthread_mutex_unlock(&vote_lock);

    char buffer[SERIALIZED_BLOCK_SIZE];
    char message[SERIALIZED_BLOCK_SIZE + 32];

    serialize_block(block, buffer);

    snprintf(message, sizeof(message),
             "PROPOSE_BLOCK:%s\n", buffer);

    printf("Broadcasting block proposal...\n");

    broadcast_message(message);
}


/* -------------------------
   Register Vote
-------------------------- */
void register_vote(const char *vote)
{
    pthread_mutex_lock(&vote_lock);

    if (!proposal_active)
    {
        pthread_mutex_unlock(&vote_lock);
        return;
    }

    if (strstr(vote, "APPROVE"))
    {
        approve_votes++;
    }

    int total_nodes = get_peer_count() + 1;
    int majority = (total_nodes / 2) + 1;

    if (approve_votes >= majority)
    {
        printf("Majority reached. Committing block.\n");

        add_block(&current_proposal);

        char buffer[SERIALIZED_BLOCK_SIZE];
        char message[SERIALIZED_BLOCK_SIZE + 32];

        serialize_block(&current_proposal, buffer);

        snprintf(message, sizeof(message),
                 "COMMIT_BLOCK:%s\n", buffer);

        broadcast_message(message);

        proposal_active = 0;
    }

    pthread_mutex_unlock(&vote_lock);
}


/* -------------------------
   Handle Commit
-------------------------- */
void handle_commit(const char *serialized)
{
    Block incoming;
    memset(&incoming, 0, sizeof(Block));

    if (!deserialize_block(serialized, &incoming))
        return;

    if (block_exists_by_index(incoming.index))
    {
        printf("Block %d already exists. Ignoring duplicate.\n",
               incoming.index);
        return;
    }

    printf("Committing received block %d\n",
           incoming.index);

    add_block(&incoming);
}
