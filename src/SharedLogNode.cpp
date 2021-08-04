#include <iostream>
#include <algorithm>
#include "SharedLogNode.h"

/* TODO: Documentation */
/**
 * Constructs the SharedLogNode
 * @param NodeType Specifys the type of this node (HEAD, MIDDLE or TAIL)
 * @param hostURI String "hostname:port" where this node can be reached 
 * @param headURI String "hostname:port" of the HEAD node of the chain. If this node is the HEAD, leave it empty.
 * @param successorURI String "hostname:port" of the SUCCESSOR node of this node in the chain.
 * @param tailURI String "hostname:port" of the TAIL node of the chain. If this node is the TAIL, leave it empty.
 * @param rec Callback function which is called when a message response is received which has been created by this node */ 
SharedLogNode::SharedLogNode(NodeType nodeType, uint8_t nodeID, const char* pathToLog, string hostURI, string headURI, string successorURI, string tailURI, BenchmarkData *benchmarkData, receive_local rec):
        Nexus_{hostURI, 0, 0},
        nodeID_{nodeID},
        threaded_{false}
{
    
    if (benchmarkData->progArgs.amountThreads > 1) {
        threaded_ = true;
        /* Create threads */
        for (size_t i = 0; i < benchmarkData->progArgs.amountThreads; i++) {
	        DEBUG_MSG("SharedLogNode(Thread number/erpcID: " << std::to_string(i) << ")");
            threads_.emplace_back(make_unique<ReplicationManager>(nodeType, pathToLog, &Nexus_, i, headURI, successorURI, tailURI, *benchmarkData)); 
        }
    } else {
        /* Just create the Object */
        threads_.emplace_back(make_unique<ReplicationManager>(nodeType, pathToLog, &Nexus_, headURI, successorURI, tailURI, rec));
        threads_.front()->init();
    }
}

/* TODO: Documentation */
void SharedLogNode::read(uint64_t logOffset) {
    if (!threaded_)
        readLog(threads_.front().get(), logOffset);
}

/* TODO: Documentation */
void SharedLogNode::append(LogEntryInFlight *logEntryInFlight, size_t dataLength) {
    if (!threaded_)
        appendLog(threads_.front().get(), logEntryInFlight, dataLength);
}

/* TODO: Documentation */
void SharedLogNode::terminate(bool force) {
    for (auto& rp : threads_)
        rp->terminate(force);
}

/* TODO: Documentation */
void SharedLogNode::get_thread_ready() {
    for ( auto& rp : threads_) { 
        unique_lock<mutex> lk(rp->threadSync_.m);
        rp->threadSync_.cv.wait(lk, [&rp]{return rp->threadSync_.threadReady;});
    }
}

void SharedLogNode::get_results(BenchmarkData *benchmarkData) {
    for ( auto& rp : threads_) {
        benchmarkData->amountAppendsSent += rp->benchmarkData_.amountAppendsSent;
        benchmarkData->amountReadsSent += rp->benchmarkData_.amountReadsSent;
        benchmarkData->totalMessagesProcessed += rp->benchmarkData_.totalMessagesProcessed;
        benchmarkData->amountAppendsSent += rp->benchmarkData_.amountAppendsSent;
        benchmarkData->amountReadsSent += rp->benchmarkData_.amountReadsSent;
    }
}

/* TODO: Documentation */
void SharedLogNode::sync(int numberOfRuns) {
    if(!threaded_)
        threads_.front().get()->networkManager_->sync(numberOfRuns);
}
