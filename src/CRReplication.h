#ifndef REPLICATIONNODE_CRReplication_H
#define REPLICATIONNODE_CRReplication_H

#include <stdio.h>
#include <string>
#include "common_info.h"
#include "common_benchmark.h"
#include "Log.h"
#include "NetworkManager.h"
using namespace std;

template<class Replication>
class NetworkManager;

class CRReplication {
    friend NetworkManager<CRReplication>;

    private:
        bool chainReady_;
        Message *setupMessage_;
        volatile bool waitForTerminateResponse_{false};
        std::thread thread_;
        Log log_;
        static void run_active(CRReplication *rp, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI);
        static void run_passive(CRReplication *rp, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI);
        void init();
        void setup(Message *message);
        void setup_response(Message *message); 
        void append_response(Message *message);
        void read_response(Message *message);
        void terminate(Message *message);
        void terminate_response(Message *message);
        void receive_locally(Message *message);

    public:
        CRReplication(NodeType nodeType, const char* pathToLog, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI, BenchmarkData benchmarkData);
        void append(Message *message);
        void read(Message *message);
        void join(bool force);

        static atomic<uint64_t> softCounter_;
        NodeType nodeType_;
        uint64_t appendsTotal{0};
        BenchmarkData benchmarkData_;
        uint64_t readsTotal{0};
        unique_ptr<NetworkManager<CRReplication>> networkManager_;

        struct ThreadSync {
            volatile bool threadReady{false};
            condition_variable cv;
            mutex m;
        } threadSync_;
};

#endif // REPLICATIONNODE_CRReplication_H
