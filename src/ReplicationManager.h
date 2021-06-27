#ifndef REPLICATIONNODE_REPLICATIONMANAGER_H
#define REPLICATIONNODE_REPLICATIONMANAGER_H

#include <stdio.h>
#include "NetworkManager.h"
#include "common_info.h"
#include <string>
using namespace std;

enum nodeType {
    HEAD,
    MIDDLE,
    TAIL
};

class NetworkManager;

// Creates and holds connections to the other nodes
class ReplicationManager {
private:
    nodeType node_;
    uint64_t softCounter_;

public:
    ReplicationManager(nodeType node, std::string hostname, int port, std::string hostnameSuccessor, int portSuccessor); 
    void append(void *reqBuffer, uint64_t reqBufferLength); 
    int read(void *reqBuffer, void *respBuffer);

    NetworkManager *NetworkManager_;
};

#endif // REPLICATIONNODE_REPLICATIONMANAGER_H