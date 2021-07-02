#include <iostream>
#include <unistd.h>
#include "ReplicationManager.h"

/* Init static softCounter */
uint64_t ReplicationManager::softCounter_ = 0;

ReplicationManager::ReplicationManager(NodeType NodeType, string hostURI, string headURI, string successorURI, string tailURI, receive_local rec): 
        Log_{POOL_SIZE, LOG_BLOCK_TOTAL_SIZE, POOL_PATH}, 
        NodeType_{NodeType},
        rec{rec},
        NetworkManager_{new NetworkManager(hostURI, headURI, successorURI, tailURI, this)}
{
    NetworkManager_->connect();
}

void ReplicationManager::wait_for_init() {
    NetworkManager_->wait_for_init();
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
            /* Send APPEND to next node in chain */
            NetworkManager_->send_message(SUCCESSOR, message);
        }; break;
        case MIDDLE: 
        {
            LogEntryInFlight *logEntryInFlight = (LogEntryInFlight *) message->reqBuffer->buf;
            /* Append the log entry to the local log */
            Log_.append(logEntryInFlight->logOffset, &logEntryInFlight->logEntry);
            /* Send APPEND to next node in chain */
            NetworkManager_->send_message(SUCCESSOR, message);
        }; break;
        case TAIL: 
        {
            LogEntryInFlight *logEntryInFlight = (LogEntryInFlight *) message->reqBuffer->buf;
            message->logOffset = logEntryInFlight->logOffset;
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
            DEBUG_MSG("ReplicationManager.read(LogEntryInFlight: logOffset: " << std::to_string(((LogEntryInFlight *) message->reqBuffer->buf)->logOffset) << " ; dataLength: " << std::to_string(((LogEntryInFlight *) message->reqBuffer->buf)->logEntry.dataLength) << " ; data: " << ((LogEntryInFlight *) message->reqBuffer->buf)->logEntry.data << ")");
            message->logOffset = ((LogEntryInFlight *) message->reqBuffer->buf)->logOffset;
            /* Send READ request drectly to TAIL */
            NetworkManager_->send_message(TAIL, message);
        }; break;
        case TAIL:
        {
            LogEntryInFlight *logEntryInFlight = (LogEntryInFlight *) message->reqBuffer->buf;
            DEBUG_MSG("ReplicationManager.read(LogEntryInFlight: logOffset: " << std::to_string(((LogEntryInFlight *) message->reqBuffer->buf)->logOffset) << " ; dataLength: " << std::to_string(((LogEntryInFlight *) message->reqBuffer->buf)->logEntry.dataLength) << " ; data: " << ((LogEntryInFlight *) message->reqBuffer->buf)->logEntry.data << ")");
            void *logEntry = Log_.read(logEntryInFlight->logOffset, &logEntrySize);
            // TODO: Check respBufferSize == 0, if read was successful
            message->logOffset = logEntryInFlight->logOffset;
            message->respBufferSize = logEntrySize;
            memcpy(message->respBuffer.buf, logEntry, logEntrySize);
            /* Send READ response */
            NetworkManager_->receive_response(message);
        }; 
    }
}
