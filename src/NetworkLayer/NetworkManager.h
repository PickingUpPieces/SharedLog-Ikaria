#ifndef REPLICATIONNODE_NETWORKMANAGER_H
#define REPLICATIONNODE_NETWORKMANAGER_H

#include <stdio.h>
#include <string>
#include "rpc.h"
#include "common_info.h"
#include "Inbound.h"
#include "Outbound.h"
using namespace std;
#ifdef CR
#include "CRReplication.h"
#define REPLICATION CRReplication
#elif CRAQ
#include "CRAQReplication.h"
#define REPLICATION CRAQReplication
#endif
class REPLICATION;
class Inbound;
class Outbound;

// Creates and holds connections to the other nodes
class NetworkManager {
    friend REPLICATION;
    friend void req_handler(erpc::ReqHandle *req_handle, void *context);
    friend void cont_func(void *context, void *tag);

    private:
        NodeType nodeType_;
        uint8_t erpcID_;
        REPLICATION *replicationManager_;
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
        NetworkManager(NodeType nodeType, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI, REPLICATION *CRAQReplication);
        void send_message(NodeType targetNode, Message *message); 
        void sync(int numberOfRuns);

        erpc::Rpc<erpc::CTransport> rpc_;
        size_t messagesInFlight_{0};
        size_t totalMessagesProcessed_{0};
};

#endif //REPLICATIONNODE_NETWORKMANAGER_H
