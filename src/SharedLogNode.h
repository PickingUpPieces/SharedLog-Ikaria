#ifndef SHAREDLOGNODE_H
#define SHAREDLOGNODE_H

#include <stdio.h>
#include <string>
#include <condition_variable>
#include "rpc.h"
#include "common_info.h"
#include "common_tests.h"
#ifdef CR
#include "CRReplication.h"
#else 
#include "CRAQReplication.h"
#endif
using namespace std;

template<class Replication>
class SharedLogNode {

    private:
        std::vector<unique_ptr<Replication>> threads_;
        erpc::Nexus Nexus_;
        uint8_t nodeID_;

    public:
        SharedLogNode(NodeType NodeType, uint8_t nodeID, const char* pathToLog, string hostURI, string headURI, string successorURI, string tailURI, BenchmarkData *benchmarkData);
        void get_thread_ready();
        void get_results(BenchmarkData *benchmarkData);
        void terminate(bool force);
};

#include "SharedLogNode.cpp"

#endif // SHAREDLOGNODE_H