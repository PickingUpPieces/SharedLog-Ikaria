#ifndef REPLICATIONNODE_CRReplication_H
#define REPLICATIONNODE_CRReplication_H

#include <stdio.h>
#include <string>
#include "common_info.h"
#include "common_tests.h"
#include "Log.h"
#include "NetworkManager.h"
using namespace std;

class NetworkManager;

class CRReplication {
    friend NetworkManager;

    private:
        bool chainReady_;
        Message *setupMessage_;
        bool waitForTerminateResponse_{false};
        std::thread thread_;
        static void run_active(CRReplication *rp, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI);
        static void run_passive(CRReplication *rp, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI);
        void setup(Message *message);
        void setup_response(Message *message); 
        void append_response(Message *message);
        void read_response(Message *message);
        void receive_locally(Message *message);

    public:
        CRReplication(NodeType nodeType, const char* pathToLog, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI, BenchmarkData benchmarkData);
        void init();
        void append(Message *message);
        void read(Message *message);
        void join(bool force);
        void terminate(Message *message);
        void terminate_response(Message *message);

        static atomic<uint64_t> softCounter_;
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

#endif // REPLICATIONNODE_CRReplication_H
