#ifndef SHAREDLOGNODE_H
#define SHAREDLOGNODE_H

#include <iostream>
#include <algorithm>
#include "SharedLogNode.h"
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

        SharedLogNode(NodeType nodeType, uint8_t nodeID, const char* pathToLog, string hostURI, string headURI, string successorURI, string tailURI, BenchmarkData *benchmarkData):
                Nexus_{hostURI, 0, 0},
                nodeID_{nodeID}
        {
            /* Create threads */
            for (size_t i = 0; i < benchmarkData->progArgs.amountThreads; i++) {
        	    DEBUG_MSG("SharedLogNode(Thread number/erpcID: " << std::to_string(i) << ")");
                threads_.emplace_back(make_unique<Replication>(nodeType, pathToLog, &Nexus_, i, headURI, successorURI, tailURI, *benchmarkData)); 
            }
        }

        void terminate(bool force) {
            for (auto& rp : threads_)
                rp->terminate(force);
        }

        void get_thread_ready() {
            for ( auto& rp : threads_) { 
                unique_lock<mutex> lk(rp->threadSync_.m);
                rp->threadSync_.cv.wait(lk, [&rp]{return rp->threadSync_.threadReady;});
            }
        }

        void get_results(BenchmarkData *benchmarkData) {
            for ( auto& rp : threads_) {
                benchmarkData->amountAppendsSent += rp->benchmarkData_.amountAppendsSent;
                benchmarkData->amountReadsSent += rp->benchmarkData_.amountReadsSent;
                benchmarkData->totalMessagesProcessed += rp->benchmarkData_.totalMessagesProcessed;
            }
        }
        };
#endif // SHAREDLOGNODE_H