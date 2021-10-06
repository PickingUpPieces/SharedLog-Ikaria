#ifndef REPLICATIONNODE_INBOUND_H
#define REPLICATIONNODE_INBOUND_H

#include <stdio.h>
#include <cstdint>
#include "rpc.h"
#include "common_info.h"
#include "NetworkManager.h"
using namespace std;

template<class Replication>
class NetworkManager;

// Server Object
template<class Replication>
class Inbound {
    friend void req_handler(erpc::ReqHandle *req_handle, void *context);
    friend NetworkManager<Replication>;

    private:
        NodeType nodeType_;
        NetworkManager<Replication> *networkManager_;
        void send_response(Message *message);

    public:
        Inbound(NodeType nodeType, erpc::Nexus *nexus, NetworkManager<Replication> *networkManager);
};

#endif //REPLICATIONNODE_INBOUND_H