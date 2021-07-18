#include <iostream>
#include "SharedLogNode.h"

/**
 * Constructs the SharedLogNode
 * @param NodeType Specifys the type of this node (HEAD, MIDDLE or TAIL)
 * @param hostURI String "hostname:port" where this node can be reached 
 * @param headURI String "hostname:port" of the HEAD node of the chain. If this node is the HEAD, leave it empty.
 * @param successorURI String "hostname:port" of the SUCCESSOR node of this node in the chain.
 * @param tailURI String "hostname:port" of the TAIL node of the chain. If this node is the TAIL, leave it empty.
 * @param rec Callback function which is called when a message response is received which has been created by this node
*/ SharedLogNode::SharedLogNode(NodeType NodeType, string hostURI, string headURI, string successorURI, string tailURI, int numberOfThreads, receive_local rec):
        Nexus_{hostURI, 0, 0},
        NodeType_{NodeType},
        threaded_{false},
        numberOfThreads_{numberOfThreads},
        roundRobinCounter_{0}
{
    if (numberOfThreads) {
        threaded_ = true;
        for (int i = 0; i < numberOfThreads; i++) {
            threads_.push_back(new ReplicationManager(NodeType, &Nexus_, i, headURI, successorURI, tailURI, true, rec));
        }
    } else
        threads_.push_back(new ReplicationManager(NodeType, &Nexus_, 0, headURI, successorURI, tailURI, false, rec));
}

/* TODO: Documentation */
void SharedLogNode::terminate_threads() {
    for ( ReplicationManager *rp : threads_)
        rp->terminate();
}

/* TODO: Documentation */
void SharedLogNode::send_message(Message *message) {
    ReplicationManager *rp;

    /* Get the right thread/ReplicationManager */
    if(threaded_) {
        rp = threads_.at(static_cast<uint>(roundRobinCounter_));
        roundRobinCounter_ = (roundRobinCounter_ + 1) % numberOfThreads_;
    } else {
        rp = threads_.front();
    }

    /* Send the message */
    if (NodeType_ == HEAD ) {
        switch (message->messageType)
        {
            case READ:
                rp->read(message);
                break;
            case APPEND:
                rp->append(message);
                break;
            default:
                break;
        }
    } else 
        rp->NetworkManager_->send_message(HEAD, message);
} 