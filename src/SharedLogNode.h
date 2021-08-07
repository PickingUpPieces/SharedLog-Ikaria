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

template<class Replication>
class SharedLogNode {
    friend ReplicationManager;

    private:
        std::vector<unique_ptr<Replication>> threads_;
        erpc::Nexus Nexus_;
        uint8_t nodeID_;

    public:
        SharedLogNode(NodeType NodeType, uint8_t nodeID, const char* pathToLog, string hostURI, string headURI, string successorURI, string tailURI, BenchmarkData *benchmarkData);
        template<typename Replication>
SharedLogNode<Replication>::SharedLogNode(NodeType nodeType, uint8_t nodeID, const char* pathToLog, string hostURI, string headURI, string successorURI, string tailURI, BenchmarkData *benchmarkData):
        Nexus_{hostURI, 0, 0},
        nodeID_{nodeID}
{
    /* Create threads */
    for (size_t i = 0; i < benchmarkData->progArgs.amountThreads; i++) {
	    DEBUG_MSG("SharedLogNode(Thread number/erpcID: " << std::to_string(i) << ")");
        threads_.emplace_back(make_unique<Replication>(nodeType, pathToLog, &Nexus_, i, headURI, successorURI, tailURI, *benchmarkData)); 
    }
}
        void get_thread_ready();
        void get_results(BenchmarkData *benchmarkData);
        void terminate(bool force);
};


#endif // SHAREDLOGNODE_H