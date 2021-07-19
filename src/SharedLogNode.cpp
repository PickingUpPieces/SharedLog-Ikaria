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
    } else {
        threads_.push_back(new ReplicationManager(NodeType, &Nexus_, 0, headURI, successorURI, tailURI, false, rec));
        threads_.front()->init();
    }
}

/* TODO: Documentation */
void SharedLogNode::terminate_threads() {
    for ( ReplicationManager *rp : threads_)
        rp->terminate();
}

/* TODO: Documentation */
void SharedLogNode::read(uint64_t logOffset) {
    /* Get the right thread/ReplicationManager */
    ReplicationManager *rp = threads_.at(static_cast<uint>(roundRobinCounter_));
    roundRobinCounter_ = (roundRobinCounter_ + 1) % numberOfThreads_;

    /* Allocate message struct */
    Message *message = (Message *) malloc(sizeof(Message));
    erpc::MsgBuffer *reqRead = (erpc::MsgBuffer *) malloc(sizeof(erpc::MsgBuffer));
    *reqRead = rp->NetworkManager_->rpc_.alloc_msg_buffer_or_die(MAX_MESSAGE_SIZE);

    /* Fill message struct */
    message->reqBuffer = reqRead;
	message->respBuffer = rp->NetworkManager_->rpc_.alloc_msg_buffer_or_die(MAX_MESSAGE_SIZE);
	message->respBufferSize = MAX_MESSAGE_SIZE;
    message->sentByThisNode = true;
    message->logOffset = logOffset;
    message->messageType = READ;

    /* Fill request data */
    uint64_t *reqPointer = (uint64_t *) message->reqBuffer->buf;
    *reqPointer = message->logOffset;
    message->reqBufferSize = sizeof(uint64_t);

    /* WORKAROUND resizing problem */
    #if NodeType_ != HEAD
    	if (message->reqBufferSize < 969)
    	    rp->NetworkManager_->rpc_.resize_msg_buffer(message->reqBuffer, 969);
    	else
    	    rp->NetworkManager_->rpc_.resize_msg_buffer(message->reqBuffer, message->reqBufferSize);
    #else
        rp->NetworkManager_->rpc_.resize_msg_buffer(message->reqBuffer, message->reqBufferSize);
    #endif

    /* Send the message */
    if (NodeType_ == HEAD)
        rp->read(message);
    else 
        rp->NetworkManager_->send_message(HEAD, message);
}

/* TODO: Documentation */
void SharedLogNode::append(void *data, size_t dataLength) {
    /* Get the right thread/ReplicationManager */
    ReplicationManager *rp = threads_.at(static_cast<uint>(roundRobinCounter_));
    roundRobinCounter_ = (roundRobinCounter_ + 1) % numberOfThreads_;

    /* Allocate message struct */
    Message *message = (Message *) malloc(sizeof(Message));
    erpc::MsgBuffer *reqRead = (erpc::MsgBuffer *) malloc(sizeof(erpc::MsgBuffer));
    *reqRead = rp->NetworkManager_->rpc_.alloc_msg_buffer_or_die(MAX_MESSAGE_SIZE);

    /* Fill message struct */
    message->reqBuffer = reqRead;
    message->reqBufferSize = MAX_MESSAGE_SIZE;
	message->respBuffer = rp->NetworkManager_->rpc_.alloc_msg_buffer_or_die(MAX_MESSAGE_SIZE);
	message->respBufferSize = MAX_MESSAGE_SIZE;
    message->sentByThisNode = true;
    message->messageType = APPEND;

    /* Fill request data */
    memcpy(message->reqBuffer->buf, data, dataLength);
    message->reqBufferSize = dataLength;

    /* WORKAROUND resizing problem */
    #if NodeType_ != HEAD
    	if (message->reqBufferSize < 969)
    	    rp->NetworkManager_->rpc_.resize_msg_buffer(message->reqBuffer, 969);
    	else
    	    rp->NetworkManager_->rpc_.resize_msg_buffer(message->reqBuffer, message->reqBufferSize);
    #else
        rp->NetworkManager_->rpc_.resize_msg_buffer(message->reqBuffer, message->reqBufferSize);
    #endif

    DEBUG_MSG("sharedLogNode.append(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");

    /* Send the message */
    if (NodeType_ == HEAD)
        rp->append(message);
    else 
        rp->NetworkManager_->send_message(HEAD, message);
} 

void SharedLogNode::sync(int numberOfRuns) {
    if(!threaded_)
        threads_.front()->NetworkManager_->sync(numberOfRuns);
}

uint64_t SharedLogNode::validate_log(string *randomString, bool logsSavedWithLogOffset) {
    if(!threaded_)
        return (threads_.front()->Log_.validate_log(randomString, logsSavedWithLogOffset));

    return 0;
}