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
        Log Log_;
        bool nodeReady_;
        bool chainReady_;
        Message *setupMessage_;
        void setup(Message *message);
        void setup_response(); 

    public:
        ReplicationManager(NodeType NodeType, string hostURI, string headURI, string successorURI, string tailURI, receive_local rec);
        void append(Message *message);
        void read(Message *message);
        void init();

        static uint64_t softCounter_; /* TODO: Not thread safe */
        NodeType NodeType_;
        receive_local rec;
        NetworkManager *NetworkManager_;
};

#endif // REPLICATIONNODE_REPLICATIONMANAGER_H