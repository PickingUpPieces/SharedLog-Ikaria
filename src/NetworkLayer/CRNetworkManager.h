#ifndef REPLICATIONNODE_NETWORKMANAGER_H
#define REPLICATIONNODE_NETWORKMANAGER_H

#include <stdio.h>
#include <string>
#include "rpc.h"
#include "common_info.h"
#include "Inbound.h"
#include "Outbound.h"
#include "CRReplication.h"
using namespace std;

#define DEFAULT_RUN_EVENT_LOOP 10

class CRReplication;
class Inbound;
class Outbound;

// Creates and holds connections to the other nodes
class NetworkManager {
    friend CRReplication;
    friend void req_handler(erpc::ReqHandle *req_handle, void *context);
    friend void cont_func(void *context, void *tag);

    private:
        NodeType nodeType_;
        uint8_t erpcID_;
        CRReplication *ReplicationManager_;
        erpc::Nexus *Nexus_;
        unique_ptr<Inbound> Inbound_;
        unique_ptr<Outbound> Head_;
        shared_ptr<Outbound> Successor_;
        shared_ptr<Outbound> Tail_;
        void receive_response(Message *message);
        void receive_message(Message *message); 
        void init();
        void send_response(Message *message); 

    public:
        NetworkManager(NodeType nodeType, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI, CRReplication *replicationManager);
        void send_message(NodeType targetNode, Message *message); 
        void sync(int numberOfRuns);

        erpc::Rpc<erpc::CTransport> rpc_;
        size_t messagesInFlight_;
        size_t totalMessagesCompleted_{0};
        size_t totalMessagesProcessed_{0};
        size_t totalReadsProcessed_{0};
        size_t totalAppendsProcessed_{0};
};

#endif //REPLICATIONNODE_NETWORKMANAGER_H
