#include <iostream>
#include <unistd.h>
#include "ReplicationManager.h"

/* Init static softCounter */
static std::atomic<uint64_t> softCounter_{0}; 


/* TODO: Documentation */
/**
 * Constructs the ReplicationManager as single threaded
 * @param NodeType Specifys the type of this node (HEAD, MIDDLE or TAIL)
 * @param headURI String "hostname:port" of the HEAD node of the chain. If this node is the HEAD, leave it empty.
 * @param successorURI String "hostname:port" of the SUCCESSOR node of this node in the chain.
 * @param tailURI String "hostname:port" of the TAIL node of the chain. If this node is the TAIL, leave it empty.
 * @param rec Callback function which is called when a message response is received which has been created by this node
 */
ReplicationManager::ReplicationManager(NodeType nodeType, uint8_t nodeID, const char* pathToLog, erpc::Nexus *Nexus, string headURI, string successorURI, string tailURI, receive_local rec):
        nodeReady_{false},
        nodeID_{nodeID},
        setupMessage_{nullptr},
        threadSync_{},
        Log_{POOL_SIZE, LOG_BLOCK_TOTAL_SIZE, pathToLog}, 
        chainReady_{false},
        NodeType_{nodeType},
        rec{rec}, 
        NetworkManager_{new NetworkManager(nodeType, Nexus, 0, headURI, successorURI, tailURI, this)} { threadSync_.threadReady = true; }


/* TODO: Documentation */
/**
 * Constructs the ReplicationManager as multi threaded Object
 * @param headURI String "hostname:port" of the HEAD node of the chain. If this node is the HEAD, leave it empty.
 * @param successorURI String "hostname:port" of the SUCCESSOR node of this node in the chain.
 * @param tailURI String "hostname:port" of the TAIL node of the chain. If this node is the TAIL, leave it empty.
 * @param rec Callback function which is called when a message response is received which has been created by this node
*/ 
ReplicationManager::ReplicationManager(NodeType nodeType, uint8_t nodeID, const char* pathToLog, erpc::Nexus *Nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI, BenchmarkData benchmarkData): 
        nodeReady_{false},
        nodeID_{nodeID},
        setupMessage_{nullptr},
        threadSync_{},
        benchmarkData_{benchmarkData},
        Log_{POOL_SIZE, LOG_BLOCK_TOTAL_SIZE, pathToLog}, 
        chainReady_{false},
        NodeType_{nodeType},
        rec{nullptr} 
    {
        if (benchmarkData_.progArgs.activeMode)
            thread_ = std::thread(run_active, this, Nexus, erpcID, headURI, successorURI, tailURI); 
        else
            thread_ = std::thread(run_passive, this, Nexus, erpcID, headURI, successorURI, tailURI); 
    }


/* TODO: Documentation */
/* Active Thread function */
void ReplicationManager::run_active(ReplicationManager *rp, erpc::Nexus *Nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI) {
    rp->NetworkManager_ = new NetworkManager(rp->NodeType_, Nexus, erpcID, headURI, successorURI, tailURI, rp); 
    rp->init();

    auto logEntryInFlight = generate_random_logEntryInFlight(rp->benchmarkData_.progArgs.valueSize);

    // Append few messages so something can be read
    for(int i = 0; i < 100; i++) 
        appendLog(rp, &logEntryInFlight, logEntryInFlight.logEntry.dataLength + (2 * 8) + sizeof(MessageType));

    while(rp->NetworkManager_->messagesInFlight_)
		rp->NetworkManager_->sync(1);

    // Set threadReady to true
    unique_lock<mutex> lk(rp->threadSync_.m);
    rp->threadSync_.threadReady = true;
    lk.unlock();
    rp->threadSync_.cv.notify_all();

    // Start threads (more or less) simultaniously 
    rp->benchmarkData_.startBenchmark->lock();
    rp->benchmarkData_.startBenchmark->unlock();

    while(likely(rp->nodeReady_ && rp->benchmarkData_.remainderNumberOfRequests)) {
        if (( rand() % 100 ) < rp->benchmarkData_.progArgs.probabilityOfRead) {
	        if ( rp->benchmarkData_.highestKnownLogOffset < 1)
		        continue;

	        auto randuint = static_cast<uint64_t>(rand());
            auto randReadOffset = randuint % rp->benchmarkData_.highestKnownLogOffset; 
            logEntryInFlight.messageType = READ;
            readLog(rp, randReadOffset);
	        rp->benchmarkData_.amountReadsSent++; 
        } else {
            appendLog(rp, &logEntryInFlight, logEntryInFlight.logEntry.dataLength + (2 * 8) + sizeof(MessageType));
	        rp->benchmarkData_.amountAppendsSent++; 
        }
	    rp->benchmarkData_.remainderNumberOfRequests--; 
    }

    /* Wait for missing response messages */
    while((rp->benchmarkData_.progArgs.totalNumberOfRequests - rp->benchmarkData_.messagesInFlight) < (rp->benchmarkData_.progArgs.totalNumberOfRequests - rp->benchmarkData_.progArgs.percentileNumberOfRequests))
		rp->NetworkManager_->sync(1);
    rp->benchmarkData_.totalMessagesProcessed = rp->NetworkManager_->totalMessagesProcessed_;
}

/* TODO: Documentation */
/* Passive Thread function */
void ReplicationManager::run_passive(ReplicationManager *rp, erpc::Nexus *Nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI) {
    rp->NetworkManager_ = new NetworkManager(rp->NodeType_, Nexus, erpcID, headURI, successorURI, tailURI, rp); 
    rp->init();

    // Set threadReady to true
    unique_lock<mutex> lk(rp->threadSync_.m);
    rp->threadSync_.threadReady = true;
    lk.unlock();
    rp->threadSync_.cv.notify_all();

    if (rp->NodeType_ == HEAD)
        readLog(rp, 0);

    while(likely(rp->nodeReady_))
		rp->NetworkManager_->sync(1);

    rp->benchmarkData_.totalMessagesProcessed = rp->NetworkManager_->totalMessagesProcessed_;
}


/**
 * Handles the SETUP process for this node
*/
void ReplicationManager::init() {
    NetworkManager_->init();
    nodeReady_ = true;

    if (NodeType_ == HEAD) {
        /*  Send SETUP message down the chain */
        LogEntryInFlight logEntryInFlight{0, SETUP, {0, ""}};
        setupMessage_ = new Message();
        setupMessage_->messageType = SETUP;
        setupMessage_->sentByThisNode = false;
        setupMessage_->reqBuffer = NetworkManager_->rpc_.alloc_msg_buffer_or_die(8 + sizeof(MessageType));
        setupMessage_->reqBufferSize = 8 + sizeof(MessageType);
        setupMessage_->respBuffer = NetworkManager_->rpc_.alloc_msg_buffer_or_die(1);
        /* Fill request data */
        memcpy(setupMessage_->reqBuffer.buf, &logEntryInFlight, setupMessage_->reqBufferSize);
        NetworkManager_->send_message(SUCCESSOR, setupMessage_);

        /* Wait for SETUP response */
        while(!chainReady_)
            NetworkManager_->sync(1);

        NetworkManager_->rpc_.free_msg_buffer(setupMessage_->reqBuffer);
        NetworkManager_->rpc_.free_msg_buffer(setupMessage_->respBuffer);
    } else {
    	DEBUG_MSG("ReplicationManager.init(Wait for Setup)");
        /* Wait for the SETUP message */
        while (!setupMessage_)
            NetworkManager_->sync(1);

    	DEBUG_MSG("ReplicationManager.init(Setup arrived)");
        /* Answer/Forward SETUP message accordingly */
        if (NodeType_ == TAIL)
            NetworkManager_->send_response(setupMessage_);
        else
            NetworkManager_->send_message(SUCCESSOR, setupMessage_);

    	DEBUG_MSG("ReplicationManager.init(Wait for first APPEND)");
        /* Wait for first APPEND/READ message from the HEAD node */
        while (!chainReady_) 
            NetworkManager_->sync(1);
    }
}

/**
 * Handles an incoming SETUP message
 * @param message Message contains important meta information/pointer e.g. Request Handle, resp/req Buffers
 */
void ReplicationManager::setup(Message *message) {
    setupMessage_ = message;
}

/**
 * Handles an incoming response for a previous send out SETUP message
 * @param message Message contains important meta information/pointer e.g. Request Handle, resp/req Buffers
 */
void ReplicationManager::setup_response() {
    if (NodeType_ == HEAD)
        chainReady_ = true;
}

/**
 * Handles an incoming APPEND message
 * Depending on the NodeType the message has to be processed differently
 * @param message Message contains important meta information/pointer e.g. Request Handle, resp/req Buffers
 */
void ReplicationManager::append(Message *message) {
    DEBUG_MSG("ReplicationManager.append(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");
    /* Assumes that the HEAD only sends messages, when it received the SETUP response */
    chainReady_ = true;

    switch(NodeType_) {
        case HEAD: 
        {
            auto *reqLogEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->reqBuffer.buf);
            /* Count Sequencer up and set the log entry number */
            reqLogEntryInFlight->logOffset = softCounter_.fetch_add(1); // FIXME: Check memory relaxation of fetch_add

            /* Append the log entry to the local Log */
            Log_.append(reqLogEntryInFlight->logOffset, &reqLogEntryInFlight->logEntry);
            
            /* Send APPEND to next node in chain */
            NetworkManager_->send_message(SUCCESSOR, message);
        }; break;
        case MIDDLE: 
        {
            auto *reqLogEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->reqBuffer.buf);
            /* Append the log entry to the local log */
            Log_.append(reqLogEntryInFlight->logOffset, &reqLogEntryInFlight->logEntry);

            /* Send APPEND to next node in chain */
            NetworkManager_->send_message(SUCCESSOR, message);
        }; break;
        case TAIL: 
        {
            auto *reqLogEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->reqBuffer.buf);
            message->logOffset = reqLogEntryInFlight->logOffset;
            /* Append the log entry to the local log */
            Log_.append(reqLogEntryInFlight->logOffset, &reqLogEntryInFlight->logEntry);
            /* Add logOffset from reqBuffer to respBuffer */
            auto *respPointer = reinterpret_cast<uint64_t *>(message->respBuffer.buf);
            *respPointer = reqLogEntryInFlight->logOffset;
            message->respBufferSize = sizeof(message->logOffset);

            /* Send APPEND response */
            NetworkManager_->send_response(message);
        }; 
    }
}

/**
 * Handles an incoming READ message
 * Depending on the NodeType the message has to be processed differently
 * @param message Message contains important meta information/pointer e.g. Request Handle, resp/req Buffers
 */
void ReplicationManager::read(Message *message) {
    /* Assumes that the HEAD only sends messages, when it received the SETUP response */
    chainReady_ = true;

    switch(NodeType_) {
        case MIDDLE: ;
        case HEAD: 
        {
            #ifdef DEBUG
            message->logOffset = ((LogEntryInFlight *) message->reqBuffer.buf)->logOffset;
            #endif
            DEBUG_MSG("ReplicationManager.read(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");
            DEBUG_MSG("ReplicationManager.read(reqLogEntryInFlight: logOffset: " << std::to_string(((LogEntryInFlight *) message->reqBuffer.buf)->logOffset) << " ; dataLength: " << std::to_string(((LogEntryInFlight *) message->reqBuffer.buf)->logEntry.dataLength) << " ; data: " << ((LogEntryInFlight *) message->reqBuffer.buf)->logEntry.data << ")");
            /* Send READ request drectly to TAIL */
            NetworkManager_->send_message(TAIL, message);
        }; break;
        case TAIL:
        {
            size_t logEntrySize{0};
            auto *reqLogEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->reqBuffer.buf);
            auto *respLogEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->respBuffer.buf);

            auto *logEntry = Log_.read(reqLogEntryInFlight->logOffset, &logEntrySize);
            // TODO: Check respBufferSize == 0, if read is legit e.g. not reading an offset which hasn't been written yet
            
            /* Prepare respBuffer */
            message->respBufferSize = logEntrySize + sizeof(reqLogEntryInFlight->logOffset);
            respLogEntryInFlight->logOffset = reqLogEntryInFlight->logOffset;
            memcpy(&respLogEntryInFlight->logEntry, logEntry, logEntrySize);

            DEBUG_MSG("ReplicationManager.read(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");
            DEBUG_MSG("ReplicationManager.read(reqLogEntryInFlight: logOffset: " << std::to_string(((LogEntryInFlight *) message->reqBuffer.buf)->logOffset) << " ; dataLength: " << std::to_string(((LogEntryInFlight *) message->reqBuffer.buf)->logEntry.dataLength) << " ; data: " << ((LogEntryInFlight *) message->reqBuffer.buf)->logEntry.data << ")");
    	    DEBUG_MSG("ReplicationManager.read(respLogEntryInFlight: dataLength: " << std::to_string(((LogEntryInFlight *) message->respBuffer.buf)->logEntry.dataLength) << " ; data: " << ((LogEntryInFlight *) message->respBuffer.buf)->logEntry.data << ")");

            /* Send READ response */
            NetworkManager_->send_response(message);
        }; 
    }
}


/* TODO: Documentation */
/* Callback function when a response is received */
void ReplicationManager::receive_locally(Message *message) {
    // If single threaded
    if (rec) { 
        rec(message);
        return;
    }

	benchmarkData_.messagesInFlight--;
    
    if (message->messageType == APPEND) {
        uint64_t *returnedLogOffset = (uint64_t *) message->respBuffer.buf;
        if (benchmarkData_.highestKnownLogOffset < *returnedLogOffset)
            benchmarkData_.highestKnownLogOffset = *returnedLogOffset;
    }  
}


/**
 * Terminates the current ReplicationManager thread
 * @param force If true, forces the thread to finish
 */
void ReplicationManager::terminate(bool force) {
    if (force)
        nodeReady_ = false;

    thread_.join();
}