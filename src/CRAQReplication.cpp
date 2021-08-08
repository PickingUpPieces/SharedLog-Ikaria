#include <iostream>
#include <unistd.h>
#include "CRAQReplication.h"
#include "helperFunctions.cpp"

/* Init static softCounter */
atomic<uint64_t>  CRAQReplication::softCounter_{0}; 

/* TODO: Documentation */
/**
 * Constructs the CRAQReplication as multi threaded Object
 * @param headURI String "hostname:port" of the HEAD node of the chain. If this node is the HEAD, leave it empty.
 * @param successorURI String "hostname:port" of the SUCCESSOR node of this node in the chain.
 * @param tailURI String "hostname:port" of the TAIL node of the chain. If this node is the TAIL, leave it empty.
*/ 
CRAQReplication::CRAQReplication(NodeType nodeType, const char* pathToLog, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI, BenchmarkData benchmarkData): 
        chainReady_{false},
        setupMessage_{nullptr},
        nodeType_{nodeType},
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
void CRAQReplication::run_active(CRAQReplication *rp, erpc::Nexus *Nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI) {
    rp->networkManager_ = make_unique<NetworkManager>(rp->nodeType_, Nexus, erpcID, headURI, successorURI, tailURI, rp); 
    rp->init();

    auto logEntryInFlight = generate_random_logEntryInFlight(rp->benchmarkData_.progArgs.valueSize);
    // Append few messages so something can be read
    for(int i = 0; i < 100; i++) 
        appendLog(rp, &logEntryInFlight, (4 * 8) + logEntryInFlight.logEntry.dataLength);

    // Set threadReady to true
    unique_lock<mutex> lk(rp->threadSync_.m);
    rp->threadSync_.threadReady = true;
    lk.unlock();
    rp->threadSync_.cv.notify_all();

    // Start threads (more or less) simultaniously 
    rp->benchmarkData_.startBenchmark->lock();
    rp->benchmarkData_.startBenchmark->unlock();

    while(likely(rp->threadSync_.threadReady && ( rp->benchmarkData_.totalMessagesProcessed <= rp->benchmarkData_.remainderNumberOfRequests))) {
        if (( rand() % 100 ) < rp->benchmarkData_.progArgs.probabilityOfRead) {
	        if ( rp->benchmarkData_.highestKnownLogOffset < 1)
		        continue;

	        auto randuint = static_cast<uint64_t>(rand());
            auto randReadOffset = randuint % rp->benchmarkData_.highestKnownLogOffset; 
            logEntryInFlight.messageType = READ;
            readLog(rp, randReadOffset);
        } else {
            appendLog(rp, &logEntryInFlight, (4 * 8) + logEntryInFlight.logEntry.dataLength);
        }
    }
}

/* TODO: Documentation */
/* Passive Thread function */
void CRAQReplication::run_passive(CRAQReplication *rp, erpc::Nexus *Nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI) {
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
}


/**
 * Handles the SETUP process for this node
*/
void CRAQReplication::init() {
    networkManager_->init();

    if (nodeType_ == HEAD) {
        /*  Send SETUP message down the chain */
        LogEntryInFlight logEntryInFlight{0, SETUP, {CLEAN, 0, ""}};
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
void CRAQReplication::setup(Message *message) {
    setupMessage_ = message;
    message->respBufferSize = 1; 
}

/**
 * Handles an incoming response for a previous send out SETUP message
 * @param message Message contains important meta information/pointer e.g. Request Handle, resp/req Buffers
 */
void CRAQReplication::setup_response(Message *message) {
    if (nodeType_ == HEAD)
        chainReady_ = true;
    else if (nodeType_ == MIDDLE)
        networkManager_->send_response(message);
}

/**
 * Handles an incoming APPEND message
 * Depending on the NodeType the message has to be processed differently
 * @param message Message contains important meta information/pointer e.g. Request Handle, resp/req Buffers
 */
void CRAQReplication::append(Message *message) {
    DEBUG_MSG("CRAQReplication.append(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");
    /* Assumes that the HEAD only sends messages, when it received the SETUP response */
    chainReady_ = true;

    switch(nodeType_) {
        case HEAD: 
        {
            auto *reqLogEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->reqBuffer.buf);
            /* Count Sequencer up and set the log entry number */
            reqLogEntryInFlight->logOffset = softCounter_.fetch_add(1); // FIXME: Check memory relaxation of fetch_add
            message->logOffset = reqLogEntryInFlight->logOffset;

            /* Append the log entry to the local Log */
            log_.append(message->logOffset, &reqLogEntryInFlight->logEntry);
            
            /* Send APPEND to next node in chain */
            networkManager_->send_message(SUCCESSOR, message);
        }; break;
        case MIDDLE: 
        {
            auto *reqLogEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->reqBuffer.buf);
            /* Append the log entry to the local log */
            log_.append(message->logOffset, &reqLogEntryInFlight->logEntry);

            /* Send APPEND to next node in chain */
            networkManager_->send_message(SUCCESSOR, message);
        }; break;
        case TAIL: 
        {
            auto *reqLogEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->reqBuffer.buf);
            // Set state CLEAN, since TAIL is last node in the chain
            reqLogEntryInFlight->logEntry.state = CLEAN;
            /* Append the log entry to the local log */
            log_.append(message->logOffset, &reqLogEntryInFlight->logEntry);
            /* Add logOffset from reqBuffer to respBuffer */
            auto *respPointer = reinterpret_cast<uint64_t *>(message->respBuffer.buf);
            *respPointer = message->logOffset;
            message->respBufferSize = sizeof(message->logOffset);

            /* Send APPEND response */
            networkManager_->send_response(message);
        }; 
    }
}


void CRAQReplication::append_response(Message *message) {
    log_.update_logEntryState(message->logOffset, CLEAN); 
    if (message->sentByThisNode)
        this->receive_locally(message);
    else
        networkManager_->send_response(message);
}

/**
 * Handles an incoming READ message
 * Depending on the NodeType the message has to be processed differently
 * @param message Message contains important meta information/pointer e.g. Request Handle, resp/req Buffers
 */
void CRAQReplication::read(Message *message) {
    /* Assumes that the HEAD only sends messages, when it received the SETUP response */
    chainReady_ = true;

    switch(nodeType_) {
        case MIDDLE: ;
        case HEAD: 
        {
            // TODO: Check if logOffset < counter
            auto [logEntry, logEntryLength] = log_.read(message->logOffset);

            if (logEntry->state == CLEAN) {
                auto *respLogEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->respBuffer.buf);

                /* Prepare respBuffer */
                message->respBufferSize = logEntryLength + 2 * 8;
                respLogEntryInFlight->logOffset = message->logOffset;
                memcpy(&respLogEntryInFlight->logEntry, logEntry, logEntryLength);

                if (message->sentByThisNode) {
                    this->receive_locally(message);
                    return;
                }

                /* Send READ response */
                networkManager_->send_response(message);
            } else {
                auto *reqLogEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->reqBuffer.buf);
                reqLogEntryInFlight->messageType = GET_LOG_ENTRY_STATE;
                message->messageType = GET_LOG_ENTRY_STATE;

                /* Send GET_LOG_ENTRY_STATE request to TAIL */
                networkManager_->send_message(TAIL, message);
            }
            DEBUG_MSG("CRAQReplication.read(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");
            DEBUG_MSG("CRAQReplication.read(reqLogEntryInFlight: logOffset: " << std::to_string(((LogEntryInFlight *) message->reqBuffer.buf)->logOffset) << " ; dataLength: " << std::to_string(((LogEntryInFlight *) message->reqBuffer.buf)->logEntry.dataLength) << " ; data: " << ((LogEntryInFlight *) message->reqBuffer.buf)->logEntry.data << ")");
        }; break;
        case TAIL:
        {
            // TODO: Check if logOffset < counter
            auto *respLogEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->respBuffer.buf);

            auto [logEntry, logEntryLength] = log_.read(message->logOffset);
            
            /* Prepare respBuffer */
            message->respBufferSize = logEntryLength + 2 * 8;
            memcpy(&respLogEntryInFlight->logEntry, logEntry, logEntryLength);

            DEBUG_MSG("CRAQReplication.read(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");
    	    DEBUG_MSG("CRAQReplication.read(respLogEntryInFlight: dataLength: " << std::to_string(((LogEntryInFlight *) message->respBuffer.buf)->logEntry.dataLength) << " ; data: " << ((LogEntryInFlight *) message->respBuffer.buf)->logEntry.data << ")");

            if (message->sentByThisNode) {
                this->receive_locally(message);
                return;
            }

            /* Send READ response */
            networkManager_->send_response(message);
        }; 
    }
}

void CRAQReplication::get_log_entry_state(Message *message) {
    // TODO: Check if logOffset < counter: If true, set logEntryState ERROR in response
    auto [logEntry, logEntryLength] = log_.read(message->logOffset);

    /* Prepare respBuffer */
    auto *respLogEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->respBuffer.buf);
    message->respBufferSize = 3 * 8;
    respLogEntryInFlight->logEntry.state = logEntry->state;

    /* Send GET_LOG_ENTRY_STATE response */
    networkManager_->send_response(message);

}

void CRAQReplication::get_log_entry_state_response(Message *message) {
    auto *respLogEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->respBuffer.buf);
    if(respLogEntryInFlight->logEntry.state == CLEAN) {
        log_.update_logEntryState(message->logOffset, CLEAN); 

        networkManager_->rpc_.free_msg_buffer(message->respBuffer);
        // Alloc new respBuffer with MAX_MESSAGE_SIZE
        message->respBuffer = networkManager_->rpc_.alloc_msg_buffer(MAX_MESSAGE_SIZE);
        while(!message->respBuffer.buf) {
            networkManager_->rpc_.run_event_loop_once();
            message->respBuffer = networkManager_->rpc_.alloc_msg_buffer(MAX_MESSAGE_SIZE);
        }
        
        this->read(message);
    } else if(respLogEntryInFlight->logEntry.state == ERROR) {
        networkManager_->send_response(message);
    }
}




/* Callback function when a response is received */
void CRAQReplication::receive_locally(Message *message) {
    benchmarkData_.totalMessagesProcessed++;
    
    if (message->messageType == APPEND) {
        benchmarkData_.amountAppendsSent++; 
        auto *returnedLogOffset = reinterpret_cast<uint64_t *>(message->respBuffer.buf);
        if (benchmarkData_.highestKnownLogOffset < *returnedLogOffset)
            benchmarkData_.highestKnownLogOffset = *returnedLogOffset;
    } else if(message->messageType == READ) {
        benchmarkData_.amountReadsSent++;
    } 

    networkManager_->rpc_.free_msg_buffer(message->reqBuffer);
    networkManager_->rpc_.free_msg_buffer(message->respBuffer);
    delete message;
}


/**
 * Terminates the current CRAQReplication thread
 * @param force If true, forces the thread to finish
 */
void CRAQReplication::terminate(bool force) {
    if (force)
        threadSync_.threadReady = false;

    thread_.join();
}
