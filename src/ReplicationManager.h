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
    private:
        static uint64_t softCounter_;
        Log Log_;

    public:
        ReplicationManager(NodeType NodeType, string hostURI, string headURI, string successorURI, string tailURI, receive_local rec);
        void append(Message *message);
        void read(Message *message);
        void init(Message *message);
        void wait_for_init();

        NodeType NodeType_;
        receive_local rec;
        NetworkManager *NetworkManager_;
};

#endif // REPLICATIONNODE_REPLICATIONMANAGER_H