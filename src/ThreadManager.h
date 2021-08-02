#ifndef THREADMANAGER_H
#define THREADMANAGER_H

#include <stdio.h>
#include <string>
#include "common_info.h"
#include "common_tests.h"
#include "ReplicationManager.h"
using namespace std;

typedef void (*receive_local)(Message *message);

class ReplicationManager;
class NetworkManager;

struct ThreadSync {
    bool threadReady{false};
    condition_variable cv;
    mutex m;
};

class ThreadManager {
    private:
        uint8_t nodeID_;
        std::thread thread_;
        static void run_active(ThreadManager *tm, const char* pathToLog, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI);
        static void run_passive(ThreadManager *tm, const char* pathToLog, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI);
        static LogEntryInFlight generate_random_logEntryInFlight(uint64_t totalSize);

    public:
        // Multi Threaded
        ThreadManager(NodeType nodeType, uint8_t nodeID, const char* pathToLog, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI, BenchmarkData benchmarkData);
        // Single Threaded
        ThreadManager(NodeType nodeType, uint8_t nodeID, const char* pathToLog, erpc::Nexus *nexus, string headURI, string successorURI, string tailURI, receive_local rec);
        static void append(ThreadManager *threadManager, void *data, size_t dataLength);
        static void read(ThreadManager *threadManager, uint64_t logOffset);
        void terminate(bool force);
        void default_receive_local(Message *message);

        ReplicationManager *replicationManager_;
        ThreadSync threadSync_;
        BenchmarkData benchmarkData_;
        NodeType nodeType_;
        receive_local rec_;
};

#endif // THREADMANAGER_H
