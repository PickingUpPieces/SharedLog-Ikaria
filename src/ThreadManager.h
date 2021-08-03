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

void append(ReplicationManager *rm, void *data, size_t dataLength);
void read(ReplicationManager *rm, uint64_t logOffset);
static LogEntryInFlight generate_random_logEntryInFlight(uint64_t totalSize);

class ThreadManager {
    private:
        std::thread thread_;
        static void run_active(ThreadManager *tm, const char* pathToLog, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI);
        static void run_passive(ThreadManager *tm, const char* pathToLog, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI);

    public:
        // Multi Threaded
        ThreadManager(NodeType nodeType, const char* pathToLog, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI, BenchmarkData benchmarkData);
        // Single Threaded
        ThreadManager(NodeType nodeType, const char* pathToLog, erpc::Nexus *nexus, string headURI, string successorURI, string tailURI, receive_local rec);
        void terminate(bool force);
        void default_receive_local(Message *message);

        ReplicationManager *replicationManager_;
        ThreadSync threadSync_;
        BenchmarkData benchmarkData_;
        NodeType nodeType_;
        receive_local rec_;
};

#endif // THREADMANAGER_H
