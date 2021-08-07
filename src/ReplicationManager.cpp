#include <iostream>
#include <unistd.h>
#include "ReplicationManager.h"

/* Init static softCounter */
static std::atomic<uint64_t> softCounter_{0}; 

/* TODO: Documentation */
/**
 * Constructs the ReplicationManager as multi threaded Object
 * @param headURI String "hostname:port" of the HEAD node of the chain. If this node is the HEAD, leave it empty.
 * @param successorURI String "hostname:port" of the SUCCESSOR node of this node in the chain.
 * @param tailURI String "hostname:port" of the TAIL node of the chain. If this node is the TAIL, leave it empty.
 * @param rec Callback function which is called when a message response is received which has been created by this node
*/ 
ReplicationManager::ReplicationManager(NodeType nodeType, const char* pathToLog, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI, BenchmarkData benchmarkData): 
        chainReady_{false},
        setupMessage_{nullptr},
        nodeType_{nodeType},
        rec{nullptr}, 
        log_{POOL_SIZE, LOG_BLOCK_TOTAL_SIZE, pathToLog},
        benchmarkData_{benchmarkData}
    {
        if (benchmarkData_.progArgs.activeMode)
            thread_ = std::thread(run_active, this, nexus, erpcID, headURI, successorURI, tailURI); 
        else
            thread_ = std::thread(run_passive, this, nexus, erpcID, headURI, successorURI, tailURI); 
    }


/* TODO: Documentation */
/* Active Thread function */
void ReplicationManager::run_active(ReplicationManager *rp, erpc::Nexus *Nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI) {
    rp->networkManager_ = make_unique<NetworkManager>(rp->nodeType_, Nexus, erpcID, headURI, successorURI, tailURI, rp); 
    rp->init();

    auto logEntryInFlight = generate_random_logEntryInFlight(rp->benchmarkData_.progArgs.valueSize);
    // Append few messages so something can be read
    for(int i = 0; i < 100; i++) 
        appendLog(rp, &logEntryInFlight, logEntryInFlight.logEntry.dataLength + (2 * 8) + sizeof(MessageType));

    // Set threadReady to true
    unique_lock<mutex> lk(rp->threadSync_.m);
    rp->threadSync_.threadReady = true;
    lk.unlock();
    rp->threadSync_.cv.notify_all();

    // Start threads (more or less) simultaniously 
    rp->benchmarkData_.startBenchmark->lock();
    rp->benchmarkData_.startBenchmark->unlock();

    while(likely(rp->threadSync_.threadReady && ( rp->totalMessagesProcessed_ <= rp->benchmarkData_.remainderNumberOfRequests))) {
        if (( rand() % 100 ) < rp->benchmarkData_.progArgs.probabilityOfRead) {
	        if ( rp->benchmarkData_.highestKnownLogOffset < 1)
		        continue;

	        auto randuint = static_cast<uint64_t>(rand());
            auto randReadOffset = randuint % rp->benchmarkData_.highestKnownLogOffset; 
            logEntryInFlight.messageType = READ;
            readLog(rp, randReadOffset);
        } else {
            appendLog(rp, &logEntryInFlight, logEntryInFlight.logEntry.dataLength + (2 * 8) + sizeof(MessageType));
        }
    }

    rp->benchmarkData_.totalMessagesProcessed = rp->totalMessagesProcessed_;
    rp->benchmarkData_.amountReadsSent = rp->totalReadsProcessed_;
    rp->benchmarkData_.amountAppendsSent = rp->totalAppendsProcessed_;
}

/* TODO: Documentation */
/* Passive Thread function */
void ReplicationManager::run_passive(ReplicationManager *rp, erpc::Nexus *Nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI) {
    rp->networkManager_ = make_unique<NetworkManager>(rp->nodeType_, Nexus, erpcID, headURI, successorURI, tailURI, rp); 
    rp->init();

    // Set threadReady to true
    unique_lock<mutex> lk(rp->threadSync_.m);
    rp->threadSync_.threadReady = true;
    lk.unlock();
    rp->threadSync_.cv.notify_all();

    if (rp->nodeType_ == HEAD)
        readLog(rp, 0);

    while(likely(rp->threadSync_.threadReady))
		rp->networkManager_->sync(1);

    rp->benchmarkData_.totalMessagesProcessed = rp->networkManager_->totalMessagesProcessed_;
    rp->benchmarkData_.amountReadsSent = rp->totalReadsProcessed_;
    rp->benchmarkData_.amountAppendsSent = rp->totalAppendsProcessed_;
}


/**
 * Handles the SETUP process for this node
*/
void ReplicationManager::init() {
    networkManager_->init();

    if (nodeType_ == HEAD) {
        /*  Send SETUP message down the chain */
        LogEntryInFlight logEntryInFlight{0, SETUP, {0, ""}};
        setupMessage_ = new Message();
        setupMessage_->messageType = SETUP;
        setupMessage_->sentByThisNode = false;
        setupMessage_->reqBuffer = networkManager_->rpc_.alloc_msg_buffer_or_die(8 + sizeof(MessageType));
        setupMessage_->reqBufferSize = 8 + sizeof(MessageType);
        setupMessage_->respBuffer = networkManager_->rpc_.alloc_msg_buffer_or_die(1);
        /* Fill request data */
        memcpy(setupMessage_->reqBuffer.buf, &logEntryInFlight, setupMessage_->reqBufferSize);
        networkManager_->send_message(SUCCESSOR, setupMessage_);

        /* Wait for SETUP response */
        while(!chainReady_)
            networkManager_->sync(1);

        networkManager_->rpc_.free_msg_buffer(setupMessage_->reqBuffer);
        networkManager_->rpc_.free_msg_buffer(setupMessage_->respBuffer);
        delete setupMessage_;
    } else {
        /* Wait for the SETUP message */
        while (!setupMessage_)
            networkManager_->sync(1);

        /* Answer/Forward SETUP message accordingly */
        if (nodeType_ == TAIL)
            networkManager_->send_response(setupMessage_);
        else
            networkManager_->send_message(SUCCESSOR, setupMessage_);

        /* Wait for first APPEND/READ message from the HEAD node */
        while (!chainReady_) 
            networkManager_->sync(1);
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
    if (nodeType_ == HEAD)
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

    switch(nodeType_) {
        case HEAD: 
        {
            auto *reqLogEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->reqBuffer.buf);
            /* Count Sequencer up and set the log entry number */
            reqLogEntryInFlight->logOffset = softCounter_.fetch_add(1); // FIXME: Check memory relaxation of fetch_add

            /* Append the log entry to the local Log */
            log_.append(reqLogEntryInFlight->logOffset, &reqLogEntryInFlight->logEntry);
            
            /* Send APPEND to next node in chain */
            networkManager_->send_message(SUCCESSOR, message);
        }; break;
        case MIDDLE: 
        {
            auto *reqLogEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->reqBuffer.buf);
            /* Append the log entry to the local log */
            log_.append(reqLogEntryInFlight->logOffset, &reqLogEntryInFlight->logEntry);

            /* Send APPEND to next node in chain */
            networkManager_->send_message(SUCCESSOR, message);
        }; break;
        case TAIL: 
        {
            auto *reqLogEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->reqBuffer.buf);
            message->logOffset = reqLogEntryInFlight->logOffset;
            /* Append the log entry to the local log */
            log_.append(reqLogEntryInFlight->logOffset, &reqLogEntryInFlight->logEntry);
            /* Add logOffset from reqBuffer to respBuffer */
            auto *respPointer = reinterpret_cast<uint64_t *>(message->respBuffer.buf);
            *respPointer = reqLogEntryInFlight->logOffset;
            message->respBufferSize = sizeof(message->logOffset);

            /* Send APPEND response */
            networkManager_->send_response(message);
        }; 
    }

    totalAppendsProcessed_++;
    totalMessagesProcessed_++;
}

/**
 * Handles an incoming READ message
 * Depending on the NodeType the message has to be processed differently
 * @param message Message contains important meta information/pointer e.g. Request Handle, resp/req Buffers
 */
void ReplicationManager::read(Message *message) {
    /* Assumes that the HEAD only sends messages, when it received the SETUP response */
    chainReady_ = true;

    switch(nodeType_) {
        case MIDDLE: ;
        case HEAD: 
        {
            #ifdef DEBUG
            message->logOffset = ((LogEntryInFlight *) message->reqBuffer.buf)->logOffset;
            #endif
            DEBUG_MSG("ReplicationManager.read(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");
            DEBUG_MSG("ReplicationManager.read(reqLogEntryInFlight: logOffset: " << std::to_string(((LogEntryInFlight *) message->reqBuffer.buf)->logOffset) << " ; dataLength: " << std::to_string(((LogEntryInFlight *) message->reqBuffer.buf)->logEntry.dataLength) << " ; data: " << ((LogEntryInFlight *) message->reqBuffer.buf)->logEntry.data << ")");
            /* Send READ request drectly to TAIL */
            networkManager_->send_message(TAIL, message);
        }; break;
        case TAIL:
        {
            auto *reqLogEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->reqBuffer.buf);
            auto *respLogEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->respBuffer.buf);

            auto [logEntry, logEntryLength] = log_.read(reqLogEntryInFlight->logOffset);

            // TODO: Check respBufferSize == 0, if read is legit e.g. not reading an offset which hasn't been written yet
            
            /* Prepare respBuffer */
            message->respBufferSize = logEntryLength + sizeof(reqLogEntryInFlight->logOffset);
            respLogEntryInFlight->logOffset = reqLogEntryInFlight->logOffset;
            memcpy(&respLogEntryInFlight->logEntry, logEntry, logEntryLength);

            DEBUG_MSG("ReplicationManager.read(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");
            DEBUG_MSG("ReplicationManager.read(reqLogEntryInFlight: logOffset: " << std::to_string(((LogEntryInFlight *) message->reqBuffer.buf)->logOffset) << " ; dataLength: " << std::to_string(((LogEntryInFlight *) message->reqBuffer.buf)->logEntry.dataLength) << " ; data: " << ((LogEntryInFlight *) message->reqBuffer.buf)->logEntry.data << ")");
    	    DEBUG_MSG("ReplicationManager.read(respLogEntryInFlight: dataLength: " << std::to_string(((LogEntryInFlight *) message->respBuffer.buf)->logEntry.dataLength) << " ; data: " << ((LogEntryInFlight *) message->respBuffer.buf)->logEntry.data << ")");

            /* Send READ response */
            networkManager_->send_response(message);
        }; 
    }

    totalReadsProcessed_++;
    totalMessagesProcessed_++;
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
        auto *returnedLogOffset = reinterpret_cast<uint64_t *>(message->respBuffer.buf);
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
        threadSync_.threadReady = false;

    thread_.join();
}
