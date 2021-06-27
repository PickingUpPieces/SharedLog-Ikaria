#ifndef REPLICATIONNODE_COMMON_NETWORKLAYER_H
#define REPLICATIONNODE_COMMON_NETWORKLAYER_H

#include <stdio.h>

const size_t max_message_size = 4096;

enum messageType {
    READ = 2,
    APPEND = 3
};


#endif // REPLICATIONNODE_COMMON_NETWORKLAYER_H