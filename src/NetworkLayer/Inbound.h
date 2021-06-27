#ifndef REPLICATIONNODE_INBOUND_H
#define REPLICATIONNODE_INBOUND_H

#include "rpc.h"
#include "ReplicationManager.h"
#include <stdio.h>
#include <cstdint>
using namespace std;

class ReplicationManager;

// Server Object
class Inbound {
private:
    uint8_t erpcID_;
    void init(erpc::Nexus *nexus);
    void run_event_loop(int numberOfRuns);

public:
    erpc::Rpc<erpc::CTransport> *rpc_;
    ReplicationManager *ReplicationManager_;

    Inbound(erpc::Nexus *nexus, uint8_t erpc_id, ReplicationManager *ReplicationManager);
    void terminate();
};

#endif //REPLICATIONNODE_INBOUND_H