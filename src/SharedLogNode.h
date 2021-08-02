#ifndef SHAREDLOGNODE_H
#define SHAREDLOGNODE_H

#include <stdio.h>
#include <string>
#include <condition_variable>
#include "rpc.h"
#include "ReplicationManager.h"
#include "common_info.h"
#include "common_tests.h"
#include "helperFunctions.h"
using namespace std;

class SharedLogNode
{
private:
    std::vector<ReplicationManager *> threads_;
    erpc::Nexus Nexus_;
    uint8_t nodeID_;
    const NodeType NodeType_;
    bool threaded_;

public:
    SharedLogNode(NodeType NodeType, uint8_t nodeID, const char* pathToLog, string hostURI, string headURI, string successorURI, string tailURI, BenchmarkData *benchmarkData, receive_local rec);
    void read(uint64_t logOffset);
    void append(void *data, size_t dataLength);
    void sync(int numberOfRuns);
    void get_thread_ready();
    void get_results(BenchmarkData *benchmarkData);
    uint64_t validate_log(string *randomString, bool logsSavedWithLogOffset);
    void terminate(bool force);
};


#endif // SHAREDLOGNODE_H