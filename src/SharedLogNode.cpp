#include <iostream>
#include "SharedLogNode.h"

void appendLog(ReplicationManager *rp, void *data, size_t dataLength);
void readLog(ReplicationManager *rp, uint64_t logOffset);

/* TODO: Documentation */
/**
 * Constructs the SharedLogNode
 * @param NodeType Specifys the type of this node (HEAD, MIDDLE or TAIL)
 * @param hostURI String "hostname:port" where this node can be reached 
 * @param headURI String "hostname:port" of the HEAD node of the chain. If this node is the HEAD, leave it empty.
 * @param successorURI String "hostname:port" of the SUCCESSOR node of this node in the chain.
 * @param tailURI String "hostname:port" of the TAIL node of the chain. If this node is the TAIL, leave it empty.
 * @param rec Callback function which is called when a message response is received which has been created by this node */ SharedLogNode::SharedLogNode(NodeType NodeType, string hostURI, string headURI, string successorURI, string tailURI, int numberOfThreads, receive_local rec):
        Nexus_{hostURI, 0, 0},
        NodeType_{NodeType},
        threaded_{false},
        numberOfThreads_{numberOfThreads},
        roundRobinCounter_{0}
{
    if (numberOfThreads > 1) {
        threaded_ = true;
        /* Create threads */
        for (int i = 0; i < numberOfThreads; i++) {
	        DEBUG_MSG("SharedLogNode(Thread number/erpcID: " << std::to_string(i) << ")");
            threads_.push_back(new ReplicationManager(NodeType, &Nexus_, i, headURI, successorURI, tailURI, true, rec));
        }
    } else {
        /* Just create the Object */
        threads_.push_back(new ReplicationManager(NodeType, &Nexus_, 0, headURI, successorURI, tailURI, false, rec));
        threads_.front()->init();
    }
}

void SharedLogNode::read(uint64_t logOffset) {
    if (!threaded_)
        readLog(threads_.front(), logOffset);
}

void SharedLogNode::append(void *data, size_t dataLength) {
    if (!threaded_)
        appendLog(threads_.front(), data, dataLength);
}

/* TODO: Documentation */
void SharedLogNode::terminate(bool force) {
    for ( ReplicationManager *rp : threads_)
        rp->terminate(force);
}

/* TODO: Documentation */
void SharedLogNode::sync(int numberOfRuns) {
    if(!threaded_)
        threads_.front()->NetworkManager_->sync(numberOfRuns);
}

/* TODO: Documentation */
uint64_t SharedLogNode::validate_log(string *randomString, bool logsSavedWithLogOffset) {
    if(!threaded_)
        return (threads_.front()->Log_.validate_log(randomString, logsSavedWithLogOffset));

    return 0;
}
