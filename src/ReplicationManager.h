#ifndef REPLICATIONNODE_REPLICATIONMANAGER_H
#define REPLICATIONNODE_REPLICATIONMANAGER_H

#include <stdio.h>
#include <string>
#include "common_info.h"
#include "NetworkManager.h"
#include "Log.h"
using namespace std;

class NetworkManager;
typedef void (*receive_local)(Message *message);

class ReplicationManager {
    friend NetworkManager;

    private:
        bool nodeReady_;
        Message *setupMessage_;
        std::thread thread_;
        static void run(ReplicationManager *replicationManager, erpc::Nexus *Nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI);
        void setup(Message *message);
        void setup_response(); 
        void add_logOffset_to_data(Message *message);

    public:
        ReplicationManager(NodeType NodeType, erpc::Nexus *Nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI, bool runAsThread, receive_local rec);
        void append(Message *message);
        void read(Message *message);
        void init();
        void terminate(bool force);

        Log Log_;
        bool chainReady_;
        NodeType NodeType_;
        receive_local rec;
        NetworkManager *NetworkManager_;
};

#endif // REPLICATIONNODE_REPLICATIONMANAGER_H
