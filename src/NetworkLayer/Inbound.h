#ifndef REPLICATIONNODE_INBOUND_H
#define REPLICATIONNODE_INBOUND_H

#include "rpc.h"
#include "NetworkManager.h"
#include "common_networkLayer.h"
#include "common_info.h"
#include <stdio.h>
#include <cstdint>
using namespace std;

class NetworkManager;

// Server Object
class Inbound {

    friend void req_handler_read(erpc::ReqHandle *req_handle, void *context);
    friend void req_handler_append(erpc::ReqHandle *req_handle, void *context);

    private:
        uint8_t erpcID_;
        erpc::Rpc<erpc::CTransport> *rpc_;
        NetworkManager *NetworkManager_;
        void init(erpc::Nexus *nexus);

    public:
        Inbound(erpc::Nexus *nexus, uint8_t erpcID, NetworkManager *NetworkManager);
        void send_response(Message *message);
        void run_event_loop(int numberOfRuns);
        void terminate();
};

#endif //REPLICATIONNODE_INBOUND_H