#ifndef REPLICATIONNODE_CRAQReplication_H
#define REPLICATIONNODE_CRAQReplication_H

#include <stdio.h>
#include <string>
#include "common_info.h"
#include "common_tests.h"
#include "Log.h"
#include "CRAQNetworkManager.h"
using namespace std;

class NetworkManager;

class CRAQReplication {
    friend NetworkManager;

    private:
        bool chainReady_;
        Message *setupMessage_;
        std::thread thread_;
        static void run_active(CRAQReplication *rp, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI);
        static void run_passive(CRAQReplication *rp, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI);
        void setup(Message *message);
        void setup_response(); 
        void receive_locally(Message *message);

    public:
        // Multi Threaded
        CRAQReplication(NodeType nodeType, const char* pathToLog, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI, BenchmarkData benchmarkData);
        void init();
        void append(Message *message);
        void read(Message *message);
        void terminate(bool force);

        NodeType nodeType_;
        Log log_;
        BenchmarkData benchmarkData_;
        unique_ptr<NetworkManager> networkManager_;
        struct ThreadSync {
            bool threadReady{false};
            condition_variable cv;
            mutex m;
        } threadSync_;
};

#endif // REPLICATIONNODE_CRAQReplication_H
