#ifndef REPLICATIONNODE_CRAQReplication_H
#define REPLICATIONNODE_CRAQReplication_H

#include <stdio.h>
#include <string>
#include "common_info.h"
#include "common_tests.h"
#include "Log.h"
#include "NetworkManager.h"
using namespace std;

class NetworkManager;

class CRAQReplication {
    friend NetworkManager;

    private:
        bool chainReady_;
        Message *setupMessage_;
        bool waitForTerminateResponse_{false};
        std::thread thread_;
        Log log_;
        size_t messagesInFlight_{0};
        static void run_active(CRAQReplication *rp, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI);
        static void run_passive(CRAQReplication *rp, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI);
        void init();
        void setup(Message *message);
        void setup_response(Message *message); 
        void append_response(Message *message);
        void get_log_entry_state(Message *message);
        void get_log_entry_state_response(Message *message);
        void terminate(Message *message);
        void terminate_response(Message *message);
        void receive_locally(Message *message);

    public:
        CRAQReplication(NodeType nodeType, const char* pathToLog, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI, BenchmarkData benchmarkData);
        void append(Message *message);
        void read(Message *message);
        void join(bool force);

        static atomic<uint64_t> softCounter_;
        NodeType nodeType_;
        BenchmarkData benchmarkData_;
        unique_ptr<NetworkManager> networkManager_;

        struct ThreadSync {
            bool threadReady{false};
            condition_variable cv;
            mutex m;
        } threadSync_;
};

#endif // REPLICATIONNODE_CRAQReplication_H
