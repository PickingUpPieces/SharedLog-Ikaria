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
    uint8_t erpc_id_;
    uint8_t session_num_;
    ReplicationManager *ReplicationManager_;
    erpc::MsgBuffer reqBuffer_;
    erpc::MsgBuffer respBuffer_;

public:
    Outbound(erpc::Nexus *nexus, uint8_t erpc_id, string connect_url, ReplicationManager *ReplicationManager);
    void terminate();
    void send_message(messageType messageType, void *data, uint64_t dataLength);
    void connect(string connect_url);

    erpc::Rpc<erpc::CTransport> *rpc_;
};

#endif //REPLICATIONNODE_OUTBOUND_H