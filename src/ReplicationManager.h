#ifndef REPLICATIONNODE_REPLICATIONMANAGER_H
#define REPLICATIONNODE_REPLICATIONMANAGER_H

#include <stdio.h>
#include <string>
#include "common_info.h"
#include "NetworkManager.h"
#include "Log.h"
using namespace std;

enum NodeType {
    HEAD,
    MIDDLE,
    TAIL
};

struct LogEntryInFlight
{
    uint64_t logOffset;
    LogEntry logEntry;
};

class NetworkManager;

// Creates and holds connections to the other nodes
class ReplicationManager {
private:
    NodeType NodeType_;
    uint64_t softCounter_;
    Log Log_;

public:
    ReplicationManager(NodeType NodeType, std::string hostname, int port, std::string hostnameSuccessor, int portSuccessor); 
    void append(void *reqBuffer, uint64_t reqBufferLength); 
    int read(void *reqBuffer, void *respBuffer);

    NetworkManager *NetworkManager_;
};

#endif // REPLICATIONNODE_REPLICATIONMANAGER_H