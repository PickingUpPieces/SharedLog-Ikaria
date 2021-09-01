#include "SharedLogNode.h"

template<class Replication>
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

template<class Replication>
void SharedLogNode<Replication>::get_thread_ready() {
    for ( auto& rp : threads_) { 
        unique_lock<mutex> lk(rp->threadSync_.m);
        rp->threadSync_.cv.wait(lk, [&rp]{return rp->threadSync_.threadReady;});
    }
}

template<class Replication>
void SharedLogNode<Replication>::get_results(BenchmarkData *benchmarkData) {
    uint64_t readsTotal = 0;
    uint64_t appendsTotal = 0;
    for ( auto& rp : threads_) {
        benchmarkData->amountAppendsSent += rp->benchmarkData_.amountAppendsSent;
        benchmarkData->amountReadsSent += rp->benchmarkData_.amountReadsSent;
        benchmarkData->totalMessagesProcessed += rp->benchmarkData_.totalMessagesProcessed;
        readsTotal += rp->readsTotal;
        appendsTotal += rp->appendsTotal;
        #ifdef LATENCY
        benchmarkData->readlatency += rp->benchmarkData_.readlatency;
        benchmarkData->appendlatency += rp->benchmarkData_.appendlatency;
        #endif
    }
    benchmarkData->lastSequencerNumber = threads_.front()->softCounter_.load();
    cout << "readsTotal: " << readsTotal << endl;
    cout << "appendsTotal: " << appendsTotal << endl;
}

template<class Replication>
void SharedLogNode<Replication>::terminate(bool force) {
    for (auto& rp : threads_)
        rp->join(force);
}
