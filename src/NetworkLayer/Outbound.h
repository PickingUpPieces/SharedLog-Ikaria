#ifndef REPLICATIONNODE_OUTBOUND_H
#define REPLICATIONNODE_OUTBOUND_H

#include "rpc.h"
#include "ReplicationManager.h"
#include "common_networkLayer.h"
#include <string>
using namespace std;

class ReplicationManager;

#define DEFAULT_RUN_EVENT_LOOP 10

class Outbound {
private:
    uint8_t erpcID_;
    int8_t sessionNum_;
    erpc::Rpc<erpc::CTransport> rpc_;
    ReplicationManager *ReplicationManager_;
    erpc::MsgBuffer reqBuffer_;
    erpc::MsgBuffer respBuffer_;

public:
    Outbound(erpc::Nexus *nexus, uint8_t erpcID, string connectURI, ReplicationManager *ReplicationManager);
    void terminate();
    void send_message(messageType messageType, void *data, uint64_t dataLength);
    void connect(string connectURI);

};

#endif //REPLICATIONNODE_OUTBOUND_H