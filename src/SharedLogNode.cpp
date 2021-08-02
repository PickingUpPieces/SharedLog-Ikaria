#include <iostream>
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
        NodeType_{nodeType},
        threaded_{false}
{
    
    if (benchmarkData->progArgs.amountThreads > 1) {
        threaded_ = true;
        /* Create threads */
        for (size_t i = 0; i < benchmarkData->progArgs.amountThreads; i++) {
	        DEBUG_MSG("SharedLogNode(Thread number/erpcID: " << std::to_string(i) << ")");
            threads_.push_back(new ThreadManager(NodeType_, nodeID_, pathToLog, &Nexus_, i, headURI, successorURI, tailURI, *benchmarkData));
        }
    } else {
        /* Just create the Object */
        threads_.push_back(new ThreadManager(NodeType_, nodeID_, pathToLog, &Nexus_, headURI, successorURI, tailURI, rec));
        //threads_.front()->init();
    }
}

/* TODO: Documentation */
void SharedLogNode::read(uint64_t logOffset) {
    if (!threaded_)
        ThreadManager::read(threads_.front(), logOffset);
}

/* TODO: Documentation */
void SharedLogNode::append(void *data, size_t dataLength) {
    if (!threaded_)
        ThreadManager::append(threads_.front(), data, dataLength);
}

/* TODO: Documentation */
void SharedLogNode::terminate(bool force) {
    for ( ThreadManager *tm : threads_)
        tm->terminate(force);
}

/* TODO: Documentation */
void SharedLogNode::get_thread_ready() {
    for ( ThreadManager *tm : threads_) { 
        unique_lock<mutex> lk(tm->threadSync_.m);
        tm->threadSync_.cv.wait(lk, [tm]{return tm->threadSync_.threadReady;});
    }
}

void SharedLogNode::get_results(BenchmarkData *benchmarkData) {
    for ( ThreadManager *tm : threads_) {
        benchmarkData->amountAppendsSent += tm->benchmarkData_.amountAppendsSent;
        benchmarkData->amountReadsSent += tm->benchmarkData_.amountReadsSent;
        benchmarkData->totalMessagesProcessed += tm->benchmarkData_.totalMessagesProcessed;
    }
}

/* TODO: Documentation */
void SharedLogNode::sync(int numberOfRuns) {
    if(!threaded_)
        threads_.front()->replicationManager_->networkManager_->sync(numberOfRuns);
}
