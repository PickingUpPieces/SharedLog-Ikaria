#include <iostream>
#include <unistd.h>
#include "CRReplication.h"
#include "helperFunctions.cpp"

/* Init static softCounter */
atomic<uint64_t> CRReplication::softCounter_{0}; 

/* TODO: Documentation */
/**
 * Constructs the CRReplication as multi threaded Object
 * @param headURI String "hostname:port" of the HEAD node of the chain. If this node is the HEAD, leave it empty.
 * @param successorURI String "hostname:port" of the SUCCESSOR node of this node in the chain.
 * @param tailURI String "hostname:port" of the TAIL node of the chain. If this node is the TAIL, leave it empty.
*/ 
CRReplication::CRReplication(NodeType nodeType, const char* pathToLog, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI, BenchmarkData benchmarkData): 
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
void CRReplication::run_active(CRReplication *rp, erpc::Nexus *Nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI) {
    rp->networkManager_ = make_unique<NetworkManager>(rp->nodeType_, Nexus, erpcID, headURI, successorURI, tailURI, rp); 
    rp->init();

    auto logEntryInFlight = generate_random_logEntryInFlight(rp->benchmarkData_.progArgs.valueSize);
    // Append few messages so something can be read
    for(int i = 0; i < 100; i++) 
        send_append_message(rp, &logEntryInFlight, logEntryInFlight.logEntry.dataLength + (3 * 8));

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
            send_read_message(rp, randReadOffset);
        } else {
            send_append_message(rp, &logEntryInFlight, logEntryInFlight.logEntry.dataLength + (3 * 8));
        }
        while(rp->networkManager_->messagesInFlight_ > 10000)
            rp->networkManager_->sync(1);
    }
    if (rp->nodeType_ == HEAD)
        rp->terminate(generate_terminate_message(rp));
    else {
        while(!rp->waitForTerminateResponse_)
            rp->networkManager_->sync(1);
    }
}

/* TODO: Documentation */
/* Passive Thread function */
void CRReplication::run_passive(CRReplication *rp, erpc::Nexus *Nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI) {
    rp->networkManager_ = make_unique<NetworkManager>(rp->nodeType_, Nexus, erpcID, headURI, successorURI, tailURI, rp); 
    rp->init();

    // Set threadReady to true
    unique_lock<mutex> lk(rp->threadSync_.m);
    rp->threadSync_.threadReady = true;
    lk.unlock();
    rp->threadSync_.cv.notify_all();

    if (rp->nodeType_ == HEAD) {
        auto logEntryInFlight = generate_random_logEntryInFlight(rp->benchmarkData_.progArgs.valueSize);
        send_append_message(rp, &logEntryInFlight, (4 * 8) + logEntryInFlight.logEntry.dataLength);
    }

    while(likely(rp->threadSync_.threadReady))
		rp->networkManager_->sync(1);

    /* Terminate */
    if (rp->nodeType_ == HEAD)
        rp->terminate(generate_terminate_message(rp));
    else {
        while(!rp->waitForTerminateResponse_)
            rp->networkManager_->sync(1);
    }
}


/**
 * Handles the SETUP process for this node
*/
void CRReplication::init() {
    networkManager_->init();

    if (nodeType_ == HEAD) {
        setupMessage_ = generate_init_message(this);
        networkManager_->send_message(SUCCESSOR, setupMessage_);

        /* Wait for SETUP response */
        while(!chainReady_)
            networkManager_->sync(1);
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
void CRReplication::setup(Message *message) {
    setupMessage_ = message;
}

/**
 * Handles an incoming response for a previous send out SETUP message
 * @param message Message contains important meta information/pointer e.g. Request Handle, resp/req Buffers
 */
void CRReplication::setup_response(Message *message) {
    switch (nodeType_) {
        case HEAD:
            chainReady_ = true;
            networkManager_->rpc_.free_msg_buffer(setupMessage_->reqBuffer);
            networkManager_->rpc_.free_msg_buffer(setupMessage_->respBuffer);
            delete setupMessage_;
            break;
        case MIDDLE:
            networkManager_->send_response(message);
            break;
        case TAIL: ;
    }
}

/**
 * Handles an incoming APPEND message
 * Depending on the NodeType the message has to be processed differently
 * @param message Message contains important meta information/pointer e.g. Request Handle, resp/req Buffers
 */
void CRReplication::append(Message *message) {
    DEBUG_MSG("CRReplication.append(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");
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
            /* Append the log entry to the local log */
            log_.append(message->logOffset, &reqLogEntryInFlight->logEntry);
            /* Add logOffset from reqBuffer to respBuffer */
            auto *respLogEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->respBuffer.buf);
            respLogEntryInFlight->logOffset = message->logOffset;
            message->respBufferSize = sizeof(message->logOffset);

            /* Send APPEND response */
            networkManager_->send_response(message);
        }; 
    }
}

void CRReplication::append_response(Message *message) {
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
void CRReplication::read(Message *message) {
    /* Assumes that the HEAD only sends messages, when it received the SETUP response */
    chainReady_ = true;

    switch(nodeType_) {
        case HEAD: ;
        case MIDDLE:
        {
            #ifndef UCR
            // TODO: Check if logOffset < counter
            DEBUG_MSG("CRReplication.read(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");
            DEBUG_MSG("CRReplication.read(reqLogEntryInFlight: logOffset: " << std::to_string(((LogEntryInFlight *) message->reqBuffer.buf)->logOffset) << " ; dataLength: " << std::to_string(((LogEntryInFlight *) message->reqBuffer.buf)->logEntry.dataLength) << " ; data: " << ((LogEntryInFlight *) message->reqBuffer.buf)->logEntry.data << ")");
            /* Send READ request drectly to TAIL */
            networkManager_->send_message(TAIL, message);
        }; break;
        #else  // If UCR, just read local like the TAIL does
        }; 
        #endif
        case TAIL:
        {
            // TODO: Check if logOffset < counter
            auto *respLogEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->respBuffer.buf);

            auto [logEntry, logEntryLength] = log_.read(message->logOffset);
            
            /* Prepare respBuffer */
            message->respBufferSize = logEntryLength + 1 * 8;
            respLogEntryInFlight->logOffset = message->logOffset;
            memcpy(&respLogEntryInFlight->logEntry, logEntry, logEntryLength);

            DEBUG_MSG("CRReplication.read(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");
    	    DEBUG_MSG("CRReplication.read(respLogEntryInFlight: dataLength: " << std::to_string(((LogEntryInFlight *) message->respBuffer.buf)->logEntry.dataLength) << " ; data: " << ((LogEntryInFlight *) message->respBuffer.buf)->logEntry.data << ")");

            if (message->sentByThisNode) {
                this->receive_locally(message);
                return;
            }

            /* Send READ response */
            networkManager_->send_response(message);
        }; 
    }
}

void CRReplication::read_response(Message *message) {
   if (message->sentByThisNode)
       this->receive_locally(message);
   else
       networkManager_->send_response(message);
}

void CRReplication::terminate(Message *message) {
    threadSync_.threadReady = false;
    switch(nodeType_){
        case HEAD:
        case MIDDLE:
            networkManager_->send_message(SUCCESSOR, message);
            break;
        case TAIL:
            networkManager_->send_response(message);
            waitForTerminateResponse_ = true;
    }
    while(!waitForTerminateResponse_)
        networkManager_->sync(1);
}

void CRReplication::terminate_response(Message *message) {
    switch(nodeType_) {
        case HEAD: 
            networkManager_->rpc_.free_msg_buffer(message->reqBuffer);
            networkManager_->rpc_.free_msg_buffer(message->respBuffer);
            delete message;
            break;
        case MIDDLE: ;
        case TAIL:
            networkManager_->send_response(message);
    }
    waitForTerminateResponse_ = true;
}




/* Callback function when a response is received */
void CRReplication::receive_locally(Message *message) {
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
 * Terminates the current CRReplication thread
 * @param force If true, forces the thread to finish
 */
void CRReplication::join(bool force) {
    if (force)
        threadSync_.threadReady = false;

    thread_.join();
}
