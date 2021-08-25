#ifndef HELPERFUNCTIONS_H 
#define HELPERFUNCTIONS_H 
#include <random>
#include <string>
#include <unistd.h>
#include "rpc.h"
#include "common_info.h"
using namespace std;

#ifdef LATENCY
#include "util/latency.h"
#endif

// https://stackoverflow.com/questions/1640258/need-a-fast-random-generator-for-c
static unsigned long x=123456789, y=362436069, z=521288629;

unsigned long xorshf96(void) {          //period 2^96-1
unsigned long t;
    x ^= x << 16;
    x ^= x >> 5;
    x ^= x << 1;

   t = x;
   x = y;
   y = z;
   z = t ^ x ^ y;

  return z;
}

template<typename Replication>
Message *generate_init_message(Replication *rp) {
    /*  Send SETUP message down the chain */
    Message *message = new Message();

    message->reqBufferSize = sizeof(LogEntryInFlightHeader);
    message->reqBuffer = rp->networkManager_->rpc_.alloc_msg_buffer(message->reqBufferSize);
    while(!message->reqBuffer.buf) {
        rp->networkManager_->sync(1);
        message->reqBuffer = rp->networkManager_->rpc_.alloc_msg_buffer(message->reqBufferSize);
    }

    message->respBuffer = rp->networkManager_->rpc_.alloc_msg_buffer(MAX_MESSAGE_SIZE);
	message->respBufferSize = MAX_MESSAGE_SIZE;
    while(!message->respBuffer.buf) {
        rp->networkManager_->sync(1);
        message->respBuffer = rp->networkManager_->rpc_.alloc_msg_buffer(MAX_MESSAGE_SIZE);
    }

    /* Fill message struct */
    message->messageType = SETUP;

    /* Fill request data */
    auto logEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->reqBuffer.buf);
    logEntryInFlight->header.logOffset = 0;
    logEntryInFlight->header.messageType = message->messageType;

    return message;
}

/* Send read request */
template<typename Replication>
void send_read_message(Replication *rp, uint64_t logOffset) {
    /* Allocate message struct */
    Message *message = new Message();

    message->reqBufferSize = sizeof(LogEntryInFlightHeader);
    message->reqBuffer = rp->networkManager_->rpc_.alloc_msg_buffer(message->reqBufferSize);
    while(!message->reqBuffer.buf) {
        rp->networkManager_->sync(1);
        message->reqBuffer = rp->networkManager_->rpc_.alloc_msg_buffer(message->reqBufferSize);
    }

    message->respBuffer = rp->networkManager_->rpc_.alloc_msg_buffer(MAX_MESSAGE_SIZE);
	message->respBufferSize = MAX_MESSAGE_SIZE;
    while(!message->respBuffer.buf) {
        rp->networkManager_->sync(1);
        message->respBuffer = rp->networkManager_->rpc_.alloc_msg_buffer(MAX_MESSAGE_SIZE);
    }

    /* Fill message struct */
    message->messageType = READ;
    message->sentByThisNode = true;
    message->logOffset = logOffset;

    /* Fill request data */
    auto logEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->reqBuffer.buf);
    logEntryInFlight->header.logOffset = message->logOffset;
    logEntryInFlight->header.messageType = message->messageType;

    #ifdef LATENCY
    #ifdef CR
    message->timestamp = erpc::rdtsc();
    #endif
    #endif

    /* Send the message */
    rp->read(message);
}

/* TODO: Documentation */
template<typename Replication>
void send_append_message(Replication *rp, LogEntryInFlight *logEntryInFlight, size_t logEntryInFlightLength) {
    /* Allocate message struct */
    Message *message = new Message();
    logEntryInFlight->header.messageType = APPEND;

    message->reqBufferSize = logEntryInFlightLength;
    message->reqBuffer = rp->networkManager_->rpc_.alloc_msg_buffer(message->reqBufferSize);
    while(!message->reqBuffer.buf) {
        rp->networkManager_->sync(1);
        message->reqBuffer = rp->networkManager_->rpc_.alloc_msg_buffer(message->reqBufferSize);
    }

	message->respBufferSize = MAX_MESSAGE_SIZE;
    message->respBuffer = rp->networkManager_->rpc_.alloc_msg_buffer(MAX_MESSAGE_SIZE);
    while(!message->respBuffer.buf) {
        rp->networkManager_->sync(1);
        message->respBuffer = rp->networkManager_->rpc_.alloc_msg_buffer(MAX_MESSAGE_SIZE);
    }

    /* Fill message struct */
    message->messageType = APPEND;
    message->sentByThisNode = true;

    /* Fill request data */
    memcpy(message->reqBuffer.buf, logEntryInFlight, logEntryInFlightLength); 

    DEBUG_MSG("sharedLogNode.append(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");

    #ifdef LATENCY
    message->timestamp = erpc::rdtsc();
    #endif

    /* Send the message */
    if (rp->nodeType_ == HEAD)
        rp->append(message);
    else 
        rp->networkManager_->send_message(HEAD, message);
} 

template<typename Replication>
Message *generate_terminate_message(Replication *rp) {
    /* Allocate message struct */
    Message *message = new Message();

    message->reqBufferSize = sizeof(LogEntryInFlightHeader); 
    message->reqBuffer = rp->networkManager_->rpc_.alloc_msg_buffer(message->reqBufferSize);
    while(!message->reqBuffer.buf) {
        rp->networkManager_->sync(1);
        message->reqBuffer = rp->networkManager_->rpc_.alloc_msg_buffer(message->reqBufferSize);
    }

	message->respBufferSize = MAX_MESSAGE_SIZE;
    message->respBuffer = rp->networkManager_->rpc_.alloc_msg_buffer(message->respBufferSize);
    while(!message->respBuffer.buf) {
        rp->networkManager_->sync(1);
        message->respBuffer = rp->networkManager_->rpc_.alloc_msg_buffer(message->respBufferSize);
    }

    /* Fill message struct */
    message->messageType = TERMINATE;
    message->sentByThisNode = true;

    /* Fill request data */
    auto logEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->reqBuffer.buf);
    logEntryInFlight->header.messageType = message->messageType;

    return message;
}

/* Generate a random logEntryInFlight for sending in append requests */
LogEntryInFlight generate_random_logEntryInFlight(uint64_t totalSize){
    LogEntryInFlight logEntryInFlight;
    #ifdef CRAQ
    logEntryInFlight.logEntry.header.state = DIRTY;
    #endif
    string possibleCharacters = "123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    mt19937 generator{random_device{}()};
    uniform_int_distribution<> dist(0, possibleCharacters.size()-1);
    string randomString;
    uint64_t stringLength;

    if (totalSize <= (sizeof(LogEntryInFlightHeader) + sizeof(LogEntryHeader)))
        stringLength = 0;
    else
        stringLength = totalSize - (sizeof(LogEntryInFlightHeader) + sizeof(LogEntryHeader));

    for(uint64_t i = 0; i < stringLength; i++) {
        size_t random_index = static_cast<size_t>(dist(generator)); //get index between 0 and possible_characters.size()-1
        randomString += possibleCharacters[random_index];
    }

    logEntryInFlight.header.logOffset = 0; 
    logEntryInFlight.logEntry.header.dataLength = stringLength;
    randomString.copy(logEntryInFlight.logEntry.data, randomString.length());

    return logEntryInFlight;
}

#endif
