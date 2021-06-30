#ifndef REPLICATIONNODE_REPLICATIONMANAGER_H
#define REPLICATIONNODE_REPLICATIONMANAGER_H

#include <stdio.h>
#include <string>
#include "common_info.h"
#include "NetworkManager.h"
#include "Log.h"
using namespace std;

enum NodeType {
    HEAD,
    MIDDLE,
    TAIL
};

class NetworkManager;
typedef void (*receive_local)(Message *message);

class ReplicationManager {
    private:
        static uint64_t softCounter_;
        Log Log_;

    public:
        ReplicationManager(NodeType NodeType, std::string hostname, int port, std::string hostnameSuccessor, int portSuccessor, receive_local rec); 
        void append(Message *message);
        void read(Message *message);

        NodeType NodeType_;
        NetworkManager *NetworkManager_;
        receive_local rec;
};

#endif // REPLICATIONNODE_REPLICATIONMANAGER_H