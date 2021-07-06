#ifndef REPLICATIONNODE_NETWORKMANAGER_H
#define REPLICATIONNODE_NETWORKMANAGER_H

#include <stdio.h>
#include <string>
#include "rpc.h"
#include "common_info.h"
#include "ReplicationManager.h"
#include "Inbound.h"
#include "Outbound.h"
using namespace std;

#define DEFAULT_RUN_EVENT_LOOP 10

class Inbound;
class Outbound;
class ReplicationManager;

// Creates and holds connections to the other nodes
class NetworkManager {
    private:
        ReplicationManager *ReplicationManager_;
        erpc::Nexus Nexus_;
        Inbound *Inbound_;
        Outbound *Head_;
        Outbound *Successor_;
        Outbound *Tail_;

    public:
        NetworkManager(string hostURI, string headURI, string successorURI, string tailURI, ReplicationManager *ReplicationManager);
        void init();
        void send_message(NodeType targetNode, Message *message); 
        void send_response(Message *message); 
        void receive_message(Message *message); 
        void receive_response(Message *message);
        void sync(int numberOfRuns);

        erpc::Rpc<erpc::CTransport> rpc_;
        int messagesInFlight_;
        int totalMessagesCompleted_;
};

#endif //REPLICATIONNODE_NETWORKMANAGER_H
