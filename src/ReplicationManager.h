#ifndef REPLICATIONNODE_REPLICATIONMANAGER_H
#define REPLICATIONNODE_REPLICATIONMANAGER_H

#include <stdio.h>
#include <string>
#include "common_info.h"
#include "common_tests.h"
#include "NetworkManager.h"
#include "Log.h"
#include "helperFunctions.h"
using namespace std;

class NetworkManager;
typedef void (*receive_local)(Message *message);

struct ThreadSync {
    bool threadReady{false};
    condition_variable cv;
    mutex m;
};

class ReplicationManager {
    friend NetworkManager;

    private:
        bool nodeReady_;
        Message *setupMessage_;
        std::thread thread_;
        static void run_active(ReplicationManager *rp, erpc::Nexus *Nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI);
        static void run_passive(ReplicationManager *rp, erpc::Nexus *Nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI);
        void setup(Message *message);
        void setup_response(); 
        void receive_locally(Message *message);

    public:
        // Multi Threaded
        ReplicationManager(NodeType NodeType, const char* pathToLog, erpc::Nexus *Nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI, BenchmarkData benchmarkData);
        // Single Threaded
        ReplicationManager(NodeType NodeType, const char* pathToLog, erpc::Nexus *Nexus, string headURI, string successorURI, string tailURI, receive_local rec);
        void append(Message *message);
        void read(Message *message);
        void init();
        void terminate(bool force);

        ThreadSync threadSync_;
        BenchmarkData benchmarkData_;
        Log Log_;
        bool chainReady_;
        NodeType NodeType_;
        receive_local rec;
        unique_ptr<NetworkManager> networkManager_;
};

#endif // REPLICATIONNODE_REPLICATIONMANAGER_H
