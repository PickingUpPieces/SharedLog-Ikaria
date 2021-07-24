#ifndef SHAREDLOGNODE_H
#define SHAREDLOGNODE_H

#include <stdio.h>
#include <string>
#include "rpc.h"
#include "common_info.h"
#include "ReplicationManager.h"
using namespace std;

class SharedLogNode
{
private:
    std::vector<ReplicationManager *> threads_;
    erpc::Nexus Nexus_;
    const NodeType NodeType_;
    bool threaded_;
    int numberOfThreads_;
    int roundRobinCounter_;

public:
    SharedLogNode(NodeType NodeType, string hostURI, string headURI, string successorURI, string tailURI, int numberOfThreads, receive_local rec);
    void read(uint64_t logOffset);
    void append(void *data, size_t dataLength);
    void sync(int numberOfRuns);
    uint64_t validate_log(string *randomString, bool logsSavedWithLogOffset);
    void terminate(bool force);
};


#endif // SHAREDLOGNODE_H