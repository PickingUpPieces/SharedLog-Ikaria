#ifndef REPLICATIONNODE_NETWORKMANAGER_H
#define REPLICATIONNODE_NETWORKMANAGER_H

#include "rpc.h"
#include "ReplicationManager.h"
#include "Inbound.h"
#include "Outbound.h"
#include "common_networkLayer.h"
#include "common_info.h"
#include <stdio.h>
#include <string>
using namespace std;

class Inbound;
class Outbound;
class ReplicationManager;

// Creates and holds connections to the other nodes
class NetworkManager {
private:
    erpc::Nexus *nexus_;
    Inbound *Inbound_;
    ReplicationManager *ReplicationManager_;

public:
    NetworkManager(string inboundHostname, int inboundPort, string outboundHostname, int outboundPort, ReplicationManager *ReplicationManager);
    void send_message(Message *message);
    void receive_message(Message *message); 
    void receive_response(Message *message);
    void sync_inbound(int numberOfRuns);
    void terminate();
    // FIXME: Only here, so we can access it in main() ReplicationManager
    Outbound *Outbound_;
};

#endif //REPLICATIONNODE_NETWORKMANAGER_H