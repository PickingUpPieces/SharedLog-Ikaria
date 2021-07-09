#include <iostream>
#include <unistd.h>
#include "ReplicationManager.h"

/* Init static softCounter */
uint64_t ReplicationManager::softCounter_ = 0;

ReplicationManager::ReplicationManager(NodeType NodeType, string hostURI, string headURI, string successorURI, string tailURI, receive_local rec): 
        Log_{POOL_SIZE, LOG_BLOCK_TOTAL_SIZE, POOL_PATH}, 
        nodeReady_{false},
        chainReady_{false},
        setupMessage_{nullptr},
        NodeType_{NodeType},
        rec{rec},
        NetworkManager_{new NetworkManager(hostURI, headURI, successorURI, tailURI, this)} {}


void ReplicationManager::init() {
    NetworkManager_->init();
    nodeReady_ = true;

    if (NodeType_ == HEAD) {
        /*  Send SETUP message down the chain */
        setupMessage_ = (Message *) malloc(sizeof(Message));
        erpc::MsgBuffer reqBuffer = NetworkManager_->rpc_.alloc_msg_buffer_or_die(1);
        setupMessage_->messageType = SETUP;
        setupMessage_->sentByThisNode = false;
        setupMessage_->reqBuffer = &reqBuffer;
        setupMessage_->respBuffer = NetworkManager_->rpc_.alloc_msg_buffer_or_die(1);
        NetworkManager_->send_message(SUCCESSOR, setupMessage_);

        /* Wait for SETUP response */
        while(!chainReady_)
            NetworkManager_->sync(10);
    } else {
        /* Wait for the SETUP message */
        while (!setupMessage_)
            NetworkManager_->sync(10);

        /* Answer/Forward SETUP accordingly */
        if (NodeType_ == TAIL)
            NetworkManager_->send_response(setupMessage_);
        else
            NetworkManager_->send_message(SUCCESSOR, setupMessage_);

        /* Wait for first APPEND/READ from the HEAD */
        while (!chainReady_) 
            NetworkManager_->sync(10);
    }
}

void ReplicationManager::setup(Message *message) {
    DEBUG_MSG("ReplicationManager.setup()");
    setupMessage_ = message;
}

void ReplicationManager::setup_response() {
    if (NodeType_ == HEAD)
        chainReady_ = true;
}

void ReplicationManager::append(Message *message) {
    DEBUG_MSG("ReplicationManager.append(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");
    /* Assumes that the HEAD only sends messages, when it received the SETUP response */
    chainReady_ = true;

    switch(NodeType_) {
        case HEAD: 
        {
            LogEntryInFlight *logEntryInFlight = (LogEntryInFlight *) message->reqBuffer->buf;
            /* Count Sequencer up and set the log entry number */
            logEntryInFlight->logOffset = softCounter_;
            ++softCounter_;
            /* Append the log entry to the local log */
            Log_.append(logEntryInFlight->logOffset, &logEntryInFlight->logEntry);
            /* Set respBuffer */
            uint64_t *respPointer = (uint64_t *) message->respBuffer.buf;
            *respPointer = message->logOffset;
            message->respBufferSize = sizeof(message->logOffset);
            /* Send APPEND to next node in chain */
            NetworkManager_->send_message(SUCCESSOR, message);
        }; break;
        case MIDDLE: 
        {
            LogEntryInFlight *logEntryInFlight = (LogEntryInFlight *) message->reqBuffer->buf;
            /* Append the log entry to the local log */
            Log_.append(logEntryInFlight->logOffset, &logEntryInFlight->logEntry);
            /* Set respBuffer */
            message->respBufferSize = 1;
            /* Send APPEND to next node in chain */
            NetworkManager_->send_message(SUCCESSOR, message);
        }; break;
        case TAIL: 
        {
            LogEntryInFlight *logEntryInFlight = (LogEntryInFlight *) message->reqBuffer->buf;
            message->logOffset = logEntryInFlight->logOffset;
            /* Append the log entry to the local log */
            Log_.append(logEntryInFlight->logOffset, &logEntryInFlight->logEntry);
            /* Set respBuffer */
            message->respBufferSize = 1;
            /* Send APPEND response */
            NetworkManager_->send_response(message);
        }; 
    }
}

void ReplicationManager::read(Message *message) {
    /* Assumes that the HEAD only sends messages, when it received the SETUP response */
    chainReady_ = true;
    size_t logEntrySize = 0;

    switch(NodeType_) {
        case MIDDLE: ;
        case HEAD: 
        {
            message->logOffset = ((LogEntryInFlight *) message->reqBuffer->buf)->logOffset;
            DEBUG_MSG("ReplicationManager.read(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");
            DEBUG_MSG("ReplicationManager.read(LogEntryInFlight: logOffset: " << std::to_string(((LogEntryInFlight *) message->reqBuffer->buf)->logOffset) << " ; dataLength: " << std::to_string(((LogEntryInFlight *) message->reqBuffer->buf)->logEntry.dataLength) << " ; data: " << ((LogEntryInFlight *) message->reqBuffer->buf)->logEntry.data << ")");
            /* Send READ request drectly to TAIL */
            NetworkManager_->send_message(TAIL, message);
        }; break;
        case TAIL:
        {
            LogEntryInFlight *logEntryInFlight = (LogEntryInFlight *) message->reqBuffer->buf;
            void *logEntry = Log_.read(logEntryInFlight->logOffset, &logEntrySize);
            // TODO: Check respBufferSize == 0, if read was successful
            message->logOffset = logEntryInFlight->logOffset;
            message->respBufferSize = logEntrySize;
            memcpy(message->respBuffer.buf, logEntry, logEntrySize);
            DEBUG_MSG("ReplicationManager.read(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");
            DEBUG_MSG("ReplicationManager.read(LogEntryInFlight: logOffset: " << std::to_string(((LogEntryInFlight *) message->reqBuffer->buf)->logOffset) << " ; dataLength: " << std::to_string(((LogEntryInFlight *) message->reqBuffer->buf)->logEntry.dataLength) << " ; data: " << ((LogEntryInFlight *) message->reqBuffer->buf)->logEntry.data << ")");
            /* Send READ response */
            NetworkManager_->send_response(message);
        }; 
    }
}
