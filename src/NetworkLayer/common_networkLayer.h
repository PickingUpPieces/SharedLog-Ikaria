#ifndef REPLICATIONNODE_COMMON_NETWORKLAYER_H
#define REPLICATIONNODE_COMMON_NETWORKLAYER_H

#include <stdio.h>
#include "rpc.h"

const size_t maxMessageSize = 4096;

enum MessageType {
    READ = 2,
    APPEND = 3
};

struct Message {
    MessageType messageType;
    uint64_t logOffset;
    erpc::ReqHandle *reqHandle;
    void *reqBuffer;
    size_t reqBufferSize;
    void *respBuffer;
    size_t respBufferSize;
};

#endif // REPLICATIONNODE_COMMON_NETWORKLAYER_H