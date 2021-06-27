#ifndef REPLICATIONNODE_NETWORKMANAGER_H
#define REPLICATIONNODE_NETWORKMANAGER_H

#include "rpc.h"
#include "ReplicationManager.h"
#include "Inbound.h"
#include "Outbound.h"
#include "common_networkLayer.h"
#include <stdio.h>
#include <string>
using namespace std;

class Inbound;
class Outbound;

// Creates and holds connections to the other nodes
class NetworkManager {
private:
    erpc::Nexus *nexus_;
    Inbound *Inbound_;
    Outbound *Outbound_;

public:
    NetworkManager(string inbound_url, int inbound_port, string outbound_url, int outbound_port, ReplicationManager *ReplicationManager);
    void send_message(messageType messageType, void *data, uint64_t dataLength);
    void terminate();
};

#endif //REPLICATIONNODE_NETWORKMANAGER_H