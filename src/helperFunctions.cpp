#ifndef HELPERFUNCTIONS_H 
#define HELPERFUNCTIONS_H 
#include "rpc.h"
#include "ReplicationManager.h"

/* TODO: Documentation */
/* readLog method */
void readLog(ReplicationManager *rp, uint64_t logOffset) {
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
    #ifdef NODETYPE
        rp->NetworkManager_->rpc_.resize_msg_buffer(message->reqBuffer, message->reqBufferSize);
    #else
    	if (message->reqBufferSize < 969)
    	    rp->NetworkManager_->rpc_.resize_msg_buffer(message->reqBuffer, 969);
    	else
    	    rp->NetworkManager_->rpc_.resize_msg_buffer(message->reqBuffer, message->reqBufferSize);
    #endif

    /* Send the message */
    if (rp->NodeType_ == HEAD)
        rp->read(message);
    else 
        rp->NetworkManager_->send_message(HEAD, message);
}

/* TODO: Documentation */
void appendLog(ReplicationManager *rp, void *data, size_t dataLength) {
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
    #ifdef NODETYPE
        rp->NetworkManager_->rpc_.resize_msg_buffer(message->reqBuffer, message->reqBufferSize);
    #else
    	if (message->reqBufferSize < 969)
    	    rp->NetworkManager_->rpc_.resize_msg_buffer(message->reqBuffer, 969);
    	else
    	    rp->NetworkManager_->rpc_.resize_msg_buffer(message->reqBuffer, message->reqBufferSize);
    #endif

    DEBUG_MSG("sharedLogNode.append(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");

    /* Send the message */
    if (rp->NodeType_ == HEAD)
        rp->append(message);
    else 
        rp->NetworkManager_->send_message(HEAD, message);
} 

/* Generate a random logEntryInFlight for sending in append requests */
void generate_random_logEntryInFlight(LogEntryInFlight *logEntryInFlight, uint64_t totalSize){
    string possibleCharacters = "123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    mt19937 generator{random_device{}()};
    uniform_int_distribution<> dist(0, possibleCharacters.size()-1);
    string randomString;
    uint64_t stringLength = totalSize - 16;

    for(int i = 0; i < stringLength; i++) {
        size_t random_index = static_cast<size_t>(dist(generator)); //get index between 0 and possible_characters.size()-1
        randomString += possibleCharacters[random_index];
    }

    logEntryInFlight->logOffset = 0; 
    logEntryInFlight->logEntry.dataLength = stringLength;
    randomString.copy(logEntryInFlight->logEntry.data, randomString.length());
}

#endif 