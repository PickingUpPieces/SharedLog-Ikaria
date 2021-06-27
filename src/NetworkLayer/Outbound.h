#ifndef REPLICATIONNODE_OUTBOUND_H
#define REPLICATIONNODE_OUTBOUND_H

#include "rpc.h"
#include "common_info.h"
#include <string>
using namespace std;

class NetworkManager;

#define DEFAULT_RUN_EVENT_LOOP 10

class Outbound {
friend void cont_func_append(void *context, void *tag);
friend void cont_func_read(void *context, void *tag);

private:
    uint8_t erpcID_;
    int8_t sessionNum_;
    NetworkManager *NetworkManager_;
    erpc::MsgBuffer reqBuffer_;
    erpc::MsgBuffer respBuffer_;

public:
    Outbound(erpc::Nexus *nexus, uint8_t erpcID, string connectURI, NetworkManager *NetworkManager);
    void terminate();
    void send_message(Message *message);
    void connect(string connectURI);

    erpc::Rpc<erpc::CTransport> rpc_;
};

#endif //REPLICATIONNODE_OUTBOUND_H