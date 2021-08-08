#ifndef REPLICATIONNODE_CRReplication_H
#define REPLICATIONNODE_CRReplication_H

#include <stdio.h>
#include <string>
#include "common_info.h"
#include "common_tests.h"
#include "CRNetworkManager.h"
#include "Log.h"
using namespace std;

class NetworkManager;
typedef void (*receive_local)(Message *message);

struct ThreadSync {
    bool threadReady{false};
    condition_variable cv;
    mutex m;
};

class CRReplication {
    friend NetworkManager;

    private:
        bool chainReady_;
        Message *setupMessage_;
        std::thread thread_;
        static void run_active(CRReplication *rp, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI);
        static void run_passive(CRReplication *rp, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI);
        void setup(Message *message);
        void setup_response(); 
        void receive_locally(Message *message);

    public:
        // Multi Threaded
        CRReplication(NodeType nodeType, const char* pathToLog, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI, BenchmarkData benchmarkData);
        void init();
        void append(Message *message);
        void read(Message *message);
        void terminate(bool force);

        NodeType nodeType_;
        receive_local rec;
        Log log_;
        ThreadSync threadSync_;
        BenchmarkData benchmarkData_;
        unique_ptr<NetworkManager> networkManager_;
};

#endif // REPLICATIONNODE_CRReplication_H
