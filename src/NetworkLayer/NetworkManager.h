#ifndef REPLICATIONNODE_NETWORKMANAGER_H
#define REPLICATIONNODE_NETWORKMANAGER_H

#include <stdio.h>
#include <string>
#include "rpc.h"
#include "common_info.h"
#include "Inbound.h"
#include "Outbound.h"
using namespace std;
#include "CRReplication.h"
#include "CRAQReplication.h"

class CRAQReplication;
class CRReplication;
template<class Replication>
class Inbound;
template<class Replication>
class Outbound;

// Creates and holds connections to the other nodes
template<class Replication>
class NetworkManager {
    friend Replication;
    friend void req_handler(erpc::ReqHandle *req_handle, void *context);
    friend void cont_func(void *context, void *tag);

    private:
        NodeType nodeType_;
        uint8_t erpcID_;
        Replication *replicationManager_;
        erpc::Nexus *Nexus_;
        unique_ptr<Inbound<Replication>> Inbound_;
        unique_ptr<Outbound<Replication>> Head_;
        shared_ptr<Outbound<Replication>> Successor_;
        shared_ptr<Outbound<Replication>> Tail_;
        void receive_response(Message *message);
        void receive_message(Message *message); 
        void init();
        void send_response(Message *message); 

    public:
        NetworkManager(NodeType nodeType, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI, Replication *CRAQReplication);
        void send_message(NodeType targetNode, Message *message); 
        void sync(int numberOfRuns);

        erpc::Rpc<erpc::CTransport> rpc_;
        size_t messagesInFlight_{0};
        size_t totalMessagesProcessed_{0};
};

#endif //REPLICATIONNODE_NETWORKMANAGER_H
