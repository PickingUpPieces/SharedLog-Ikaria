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

        /* Answer/Forward SETUP message accordingly */
        if (NodeType_ == TAIL)
            NetworkManager_->send_response(setupMessage_);
        else
            NetworkManager_->send_message(SUCCESSOR, setupMessage_);

        /* Wait for first APPEND/READ message from the HEAD node */
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
            LogEntryInFlight *reqLogEntryInFlight = (LogEntryInFlight *) message->reqBuffer->buf;
            /* Count Sequencer up and set the log entry number */
	        // FIXME: No sequence number 0, when counting up before
            ++softCounter_;
            reqLogEntryInFlight->logOffset = softCounter_;
            
            // FIXME: Remove this when benchmark is on
            add_logOffset_to_data(message);

            /* Append the log entry to the local Log */
            Log_.append(reqLogEntryInFlight->logOffset, &reqLogEntryInFlight->logEntry);
            
            /* Send APPEND to next node in chain */
            NetworkManager_->send_message(SUCCESSOR, message);
        }; break;
        case MIDDLE: 
        {
            LogEntryInFlight *reqLogEntryInFlight = (LogEntryInFlight *) message->reqBuffer->buf;
            /* Append the log entry to the local log */
            Log_.append(reqLogEntryInFlight->logOffset, &reqLogEntryInFlight->logEntry);

            /* Send APPEND to next node in chain */
            NetworkManager_->send_message(SUCCESSOR, message);
        }; break;
        case TAIL: 
        {
            LogEntryInFlight *reqLogEntryInFlight = (LogEntryInFlight *) message->reqBuffer->buf;
            message->logOffset = reqLogEntryInFlight->logOffset;
            /* Append the log entry to the local log */
            Log_.append(reqLogEntryInFlight->logOffset, &reqLogEntryInFlight->logEntry);
            /* Add logOffset from reqBuffer to respBuffer */
            uint64_t *respPointer = (uint64_t *) message->respBuffer.buf;
            *respPointer = reqLogEntryInFlight->logOffset;
            message->respBufferSize = sizeof(message->logOffset);

            /* Send APPEND response */
            NetworkManager_->send_response(message);
        }; 
    }
}


void ReplicationManager::read(Message *message) {
    /* Assumes that the HEAD only sends messages, when it received the SETUP response */
    chainReady_ = true;

    switch(NodeType_) {
        case MIDDLE: ;
        case HEAD: 
        {
            message->logOffset = ((LogEntryInFlight *) message->reqBuffer->buf)->logOffset;
            DEBUG_MSG("ReplicationManager.read(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");
            DEBUG_MSG("ReplicationManager.read(reqLogEntryInFlight: logOffset: " << std::to_string(((LogEntryInFlight *) message->reqBuffer->buf)->logOffset) << " ; dataLength: " << std::to_string(((LogEntryInFlight *) message->reqBuffer->buf)->logEntry.dataLength) << " ; data: " << ((LogEntryInFlight *) message->reqBuffer->buf)->logEntry.data << ")");
            /* Send READ request drectly to TAIL */
            NetworkManager_->send_message(TAIL, message);
        }; break;
        case TAIL:
        {
            size_t logEntrySize = 0;
            LogEntryInFlight *reqLogEntryInFlight = (LogEntryInFlight *) message->reqBuffer->buf;
            LogEntryInFlight *respLogEntryInFlight = (LogEntryInFlight *) message->respBuffer.buf;

            void *logEntry = Log_.read(reqLogEntryInFlight->logOffset, &logEntrySize);
            // TODO: Check respBufferSize == 0, if read is legit e.g. not reading an offset which hasn't been written yet
            
            /* Prepare respBuffer */
            message->respBufferSize = logEntrySize + sizeof(reqLogEntryInFlight->logOffset);
            respLogEntryInFlight->logOffset = reqLogEntryInFlight->logOffset;
            memcpy(&respLogEntryInFlight->logEntry, logEntry, logEntrySize);

            DEBUG_MSG("ReplicationManager.read(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");
            DEBUG_MSG("ReplicationManager.read(reqLogEntryInFlight: logOffset: " << std::to_string(((LogEntryInFlight *) message->reqBuffer->buf)->logOffset) << " ; dataLength: " << std::to_string(((LogEntryInFlight *) message->reqBuffer->buf)->logEntry.dataLength) << " ; data: " << ((LogEntryInFlight *) message->reqBuffer->buf)->logEntry.data << ")");
    	    DEBUG_MSG("ReplicationManager.read(respLogEntryInFlight: dataLength: " << std::to_string(((LogEntryInFlight *) message->respBuffer.buf)->logEntry.dataLength) << " ; data: " << ((LogEntryInFlight *) message->respBuffer.buf)->logEntry.data << ")");

            /* Send READ response */
            NetworkManager_->send_response(message);
        }; 
    }
}


/* DEBUG functions */
/**
 * Adds the logOffset number to the data, so it can be validated later if the right entries are written in the write logOffsets.
 * @param message Message contains important meta information/pointer e.g. Request Handle, resp/req Buffers
 */
void ReplicationManager::add_logOffset_to_data(Message *message) {
    LogEntryInFlight *reqLogEntryInFlight = (LogEntryInFlight *) message->reqBuffer->buf;
    string temp = (string) reqLogEntryInFlight->logEntry.data;
    temp += std::to_string(reqLogEntryInFlight->logOffset);
    temp.copy(reqLogEntryInFlight->logEntry.data, temp.length());
    reqLogEntryInFlight->logEntry.dataLength = temp.length();
    message->logOffset = reqLogEntryInFlight->logOffset;
    message->reqBufferSize = sizeof(reqLogEntryInFlight->logOffset) + sizeof(reqLogEntryInFlight->logEntry.dataLength) + reqLogEntryInFlight->logEntry.dataLength;
    NetworkManager_->rpc_.resize_msg_buffer(message->reqBuffer, message->reqBufferSize);
}
