#ifndef HELPERFUNCTIONS_H 
#define HELPERFUNCTIONS_H 
#include <random>
#include <string>
#include <unistd.h>
#include "rpc.h"
#include "common_info.h"
using namespace std;

/* TODO: Documentation */
/* readLog method */
template<typename Replication>
void send_read_message(Replication *rp, uint64_t logOffset) {
    /* Allocate message struct */
    Message *message = new Message();

    message->reqBuffer = rp->networkManager_->rpc_.alloc_msg_buffer(8 + sizeof(MessageType));
    while(!message->reqBuffer.buf) {
        rp->networkManager_->sync(1);
        message->reqBuffer = rp->networkManager_->rpc_.alloc_msg_buffer(8 + sizeof(MessageType));
    }

    message->respBuffer = rp->networkManager_->rpc_.alloc_msg_buffer(MAX_MESSAGE_SIZE);
    while(!message->respBuffer.buf) {
        rp->networkManager_->sync(1);
        message->respBuffer = rp->networkManager_->rpc_.alloc_msg_buffer(MAX_MESSAGE_SIZE);
    }

    /* Fill message struct */
    message->messageType = READ;
	message->respBufferSize = MAX_MESSAGE_SIZE;
    message->sentByThisNode = true;
    message->logOffset = logOffset;

    /* Fill request data */
    auto logEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->reqBuffer.buf);
    logEntryInFlight->messageType = READ;
    logEntryInFlight->logOffset = message->logOffset;
    logEntryInFlight->messageType = message->messageType;
    message->reqBufferSize = 8 + sizeof(logEntryInFlight->messageType);

    /* Send the message */
    rp->read(message);
}

/* TODO: Documentation */
template<typename Replication>
void send_append_message(Replication *rp, LogEntryInFlight *logEntryInFlight, size_t dataLength) {
    /* Allocate message struct */
    Message *message = new Message();
    logEntryInFlight->messageType = APPEND;

    message->reqBuffer = rp->networkManager_->rpc_.alloc_msg_buffer(dataLength + 4 * 8);
    while(!message->reqBuffer.buf) {
        rp->networkManager_->sync(1);
        message->reqBuffer = rp->networkManager_->rpc_.alloc_msg_buffer(dataLength + 4 * 8);
    }

    message->respBuffer = rp->networkManager_->rpc_.alloc_msg_buffer(MAX_MESSAGE_SIZE);
    while(!message->respBuffer.buf) {
        rp->networkManager_->sync(1);
        message->respBuffer = rp->networkManager_->rpc_.alloc_msg_buffer(MAX_MESSAGE_SIZE);
    }

    /* Fill message struct */
    message->messageType = APPEND;
	message->respBufferSize = MAX_MESSAGE_SIZE;
    message->sentByThisNode = true;

    /* Fill request data */
    memcpy(message->reqBuffer.buf, logEntryInFlight, dataLength);
    message->reqBufferSize = dataLength;

    DEBUG_MSG("sharedLogNode.append(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");

    /* Send the message */
    if (rp->nodeType_ == HEAD)
        rp->append(message);
    else 
        rp->networkManager_->send_message(HEAD, message);
} 

/* Generate a random logEntryInFlight for sending in append requests */
LogEntryInFlight generate_random_logEntryInFlight(uint64_t totalSize){
    LogEntryInFlight logEntryInFlight;
    #ifdef CRAQ
    logEntryInFlight.logEntry.state = DIRTY;
    #endif
    string possibleCharacters = "123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    mt19937 generator{random_device{}()};
    uniform_int_distribution<> dist(0, possibleCharacters.size()-1);
    string randomString;
    uint64_t stringLength;

    if (totalSize <= 32)
        stringLength = 0;
    else
        stringLength = totalSize - 4 * 8;

    for(uint64_t i = 0; i < stringLength; i++) {
        size_t random_index = static_cast<size_t>(dist(generator)); //get index between 0 and possible_characters.size()-1
        randomString += possibleCharacters[random_index];
    }

    logEntryInFlight.logOffset = 0; 
    logEntryInFlight.logEntry.dataLength = stringLength;
    randomString.copy(logEntryInFlight.logEntry.data, randomString.length());

    return logEntryInFlight;
}

#endif
