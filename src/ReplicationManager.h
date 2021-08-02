#ifndef REPLICATIONNODE_REPLICATIONMANAGER_H
#define REPLICATIONNODE_REPLICATIONMANAGER_H

#include <stdio.h>
#include <string>
#include "common_info.h"
#include "NetworkManager.h"
#include "Log.h"
using namespace std;

class NetworkManager;
class ThreadManager;
typedef void (*receive_local)(Message *message);

class ReplicationManager {
    friend NetworkManager;

    private:
        bool nodeReady_;
        Message *setupMessage_;
        receive_local rec_;
        void setup(Message *message);
        void setup_response(); 

    public:
        //ReplicationManager(NodeType nodeType, const char* pathToLog, NetworkManager *networkManager);
        ReplicationManager(NodeType nodeType, const char* pathToLog, erpc::Nexus *nexus, string headURI, string successorURI, string tailURI, receive_local rec);
        void init();
        void append(Message *message);
        void read(Message *message);
        void send_message(NodeType targetNode, Message *message);
        void receive_locally(Message *message);
        void terminate();

        NetworkManager *networkManager_;
        bool chainReady_;
        Log Log_;
        NodeType NodeType_;
};

#endif // REPLICATIONNODE_REPLICATIONMANAGER_H
