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
    friend void req_handler_read(erpc::ReqHandle *req_handle, void *context);
    friend void req_handler_append(erpc::ReqHandle *req_handle, void *context);

    private:
        NetworkManager *NetworkManager_;
        void init(erpc::Nexus *nexus);

    public:
        Inbound(erpc::Nexus *nexus, NetworkManager *NetworkManager);
        void send_response(Message *message);
        void terminate();
};

#endif //REPLICATIONNODE_INBOUND_H