#ifndef REPLICATIONNODE_INBOUND_H
#define REPLICATIONNODE_INBOUND_H

#include <stdio.h>
#include <cstdint>
#include "rpc.h"
#include "common_info.h"
#include "NetworkManager.h"
using namespace std;

class NetworkManager;

// Server Object
class Inbound {
    friend void req_handler(erpc::ReqHandle *req_handle, void *context);

    private:
        NodeType nodeType_;
        NetworkManager *NetworkManager_;

    public:
        Inbound(NodeType nodeType, erpc::Nexus *nexus, NetworkManager *NetworkManager);
        void send_response(Message *message);
};

#endif //REPLICATIONNODE_INBOUND_H