#ifndef REPLICATIONNODE_INBOUND_H
#define REPLICATIONNODE_INBOUND_H

#include <stdio.h>
#include <cstdint>
#include "rpc.h"
#include "common_info.h"
#ifdef CR
#include "CRNetworkManager.h"
#else
#include "CRAQNetworkManager.h"
#endif
using namespace std;

class NetworkManager;

// Server Object
class Inbound {
    friend void req_handler(erpc::ReqHandle *req_handle, void *context);
    friend NetworkManager;

    private:
        NodeType nodeType_;
        NetworkManager *networkManager_;
        void send_response(Message *message);

    public:
        Inbound(NodeType nodeType, erpc::Nexus *nexus, NetworkManager *networkManager);
};

#endif //REPLICATIONNODE_INBOUND_H