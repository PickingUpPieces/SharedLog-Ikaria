#ifndef REPLICATIONNODE_COMMON_NETWORKLAYER_H
#define REPLICATIONNODE_COMMON_NETWORKLAYER_H

#include <stdio.h>
#include "rpc.h"

const size_t maxMessageSize = 4112;

enum MessageType {
    READ = 2,
    APPEND = 3
};

#endif // REPLICATIONNODE_COMMON_NETWORKLAYER_H