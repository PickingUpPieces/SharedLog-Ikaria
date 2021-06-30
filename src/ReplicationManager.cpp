#include <iostream>
#include <unistd.h>
#include "ReplicationManager.h"

/* Init static softCounter */
uint64_t ReplicationManager::softCounter_ = 0;

ReplicationManager::ReplicationManager(NodeType NodeType, std::string hostname, int port, std::string hostnameSuccessor, int portSuccessor, receive_local rec): 
        Log_{POOL_SIZE, LOG_BLOCK_TOTAL_SIZE, POOL_PATH}, 
        NodeType_{NodeType}
{
    this->NetworkManager_ = new NetworkManager(hostname, port, hostnameSuccessor, portSuccessor, this);
    this->rec = rec; 
}


void ReplicationManager::append(Message *message) {
    DEBUG_MSG("ReplicationManager.append(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");
    switch(NodeType_) {
        case HEAD: 
        {
            LogEntryInFlight *logEntryInFlight = (LogEntryInFlight *) message->reqBuffer->buf;
            /* Set logOffset */
            ++softCounter_;
            logEntryInFlight->logOffset = softCounter_;
            message->logOffset = softCounter_;
            /* Append the log entry to the local log */
            Log_.append(logEntryInFlight->logOffset, &logEntryInFlight->logEntry);
            /* Send append to next node in chain */
            NetworkManager_->send_message(message);
        }; break;
        case MIDDLE: break;
        case TAIL: {
            LogEntryInFlight *logEntryInFlight = (LogEntryInFlight *) message->reqBuffer->buf;
            Log_.append(logEntryInFlight->logOffset, &logEntryInFlight->logEntry);
            /* Send APPEND response */
            NetworkManager_->receive_response(message);
        };
    }
}

void ReplicationManager::read(Message *message) {
    DEBUG_MSG("ReplicationManager.read(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");
    size_t logEntrySize = 0;

    switch(NodeType_) {
        case MIDDLE: ;
        case HEAD: 
        {
            DEBUG_MSG("ReplicationManager.read(logOffset: " << std::to_string(((LogEntryInFlight *) message->reqBuffer->buf)->logOffset) << ")");
            /* Send READ request to next node in chain, to get the answer from the tail */
            // FIXME: Maybe send request directly to the tail
            NetworkManager_->send_message(message);
        }; break;
        case TAIL:
        {
            LogEntryInFlight *logEntryInFlight = (LogEntryInFlight *) message->reqBuffer->buf;
            DEBUG_MSG("ReplicationManager.read(logOffset: " << std::to_string(logEntryInFlight->logOffset) << ")");
            void *logEntry = Log_.read(logEntryInFlight->logOffset, &logEntrySize);
            // TODO: Check respBufferSize == 0, if read was successful
            message->respBufferSize = logEntrySize;
            memcpy(message->respBuffer.buf, logEntry, logEntrySize);
            /* Send READ response */
            NetworkManager_->receive_response(message);
        }; 
    }
}