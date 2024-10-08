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
        log_{POOL_SIZE, LOG_BLOCK_TOTAL_SIZE, pathToLog},
        nodeType_{nodeType},
        benchmarkData_{benchmarkData},
        #ifdef UCR
        uncommittedRead{true}
        #else
        uncommittedRead{false}
        #endif
    {
	// FIXME: Naughty race condition when starting threads in constructor
        if (benchmarkData_.progArgs.activeMode)
            thread_ = std::thread(run_active, this, nexus, erpcID, headURI, successorURI, tailURI); 
        else
            thread_ = std::thread(run_passive, this, nexus, erpcID, headURI, successorURI, tailURI); 
    }


/* TODO: Documentation */
/* Active Thread function */
void CRAQReplication::run_active(CRAQReplication *rp, erpc::Nexus *Nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI) {
    rp->networkManager_ = make_unique<NetworkManager<CRAQReplication>>(rp->nodeType_, Nexus, erpcID, headURI, successorURI, tailURI, rp); 
    rp->init();

    auto logEntryInFlight = generate_random_logEntryInFlight(rp->benchmarkData_.progArgs.valueSize);
    // Append few messages so something can be read
    for(int i = 0; i < 1000; i++) 
        send_append_message(rp, &logEntryInFlight, sizeof(LogEntryInFlightHeader) + sizeof(LogEntryHeader) + logEntryInFlight.logEntry.header.dataLength);

    uint64_t sentMessages = 1000;

    // Set threadReady to true
    unique_lock<mutex> lk(rp->threadSync_.m);
    rp->threadSync_.threadReady = true;
    lk.unlock();
    rp->threadSync_.cv.notify_all();

    // Start threads (more or less) simultaniously 
    rp->benchmarkData_.startBenchmark->lock();
    rp->benchmarkData_.startBenchmark->unlock();


    while(likely(rp->threadSync_.threadReady)) {
	    sentMessages++;

        #ifdef BENCHMARK_MAX
        // Maximum appends already inFlight -> Do read
        if ((sentMessages - rp->benchmarkData_.totalMessagesProcessed) > rp->benchmarkData_.progArgs.messageInFlightCap) {
	        auto randuint = static_cast<uint64_t>(xorshf96());
            auto randReadOffset = randuint % rp->benchmarkData_.highestKnownLogOffset; 
            send_read_message(rp, randReadOffset);
            rp->networkManager_->sync(1);
        } else {
            send_append_message(rp, &logEntryInFlight, sizeof(LogEntryInFlightHeader) + sizeof(LogEntryHeader) + logEntryInFlight.logEntry.header.dataLength);
        }
        #else
        if (( xorshf96() % 100 ) < rp->benchmarkData_.progArgs.probabilityOfRead) {
	        if ( rp->benchmarkData_.highestKnownLogOffset < 1)
		        continue;

	        auto randuint = static_cast<uint64_t>(xorshf96());

            #ifdef BENCHMARK_RANGE
            // Get random value in range
            auto randReadOffset = randuint % rp->benchmarkData_.benchmarkReadRange; 
            if ( rp->benchmarkData_.highestKnownLogOffset > randReadOffset ) {
                randReadOffset = rp->benchmarkData_.highestKnownLogOffset - randReadOffset;
            }
            #else
            auto randReadOffset = randuint % rp->benchmarkData_.highestKnownLogOffset; 
            #endif // BENCHMARK_RANGE

            send_read_message(rp, randReadOffset);
            rp->networkManager_->sync(1);
        } else {
            send_append_message(rp, &logEntryInFlight, sizeof(LogEntryInFlightHeader) + sizeof(LogEntryHeader) + logEntryInFlight.logEntry.header.dataLength);
        }

        while(((sentMessages - rp->benchmarkData_.totalMessagesProcessed) > rp->benchmarkData_.progArgs.messageInFlightCap) && (rp->threadSync_.threadReady == true))
            rp->networkManager_->sync(1);
        #endif
    }
    /* Terminate */
    if (rp->nodeType_ == HEAD)
        rp->terminate(generate_terminate_message(rp));
    else {
        while(rp->waitForTerminateResponse_ == false)
            rp->networkManager_->sync(1);
    }
}

/* TODO: Documentation */
/* Passive Thread function */
void CRAQReplication::run_passive(CRAQReplication *rp, erpc::Nexus *Nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI) {
    rp->networkManager_ = make_unique<NetworkManager<CRAQReplication>>(rp->nodeType_, Nexus, erpcID, headURI, successorURI, tailURI, rp); 
    rp->init();

    // Set threadReady to true
    unique_lock<mutex> lk(rp->threadSync_.m);
    rp->threadSync_.threadReady = true;
    lk.unlock();
    rp->threadSync_.cv.notify_all();

    if (rp->nodeType_ == HEAD) {
        auto logEntryInFlight = generate_random_logEntryInFlight(rp->benchmarkData_.progArgs.valueSize);
        send_append_message(rp, &logEntryInFlight, sizeof(LogEntryInFlightHeader) + sizeof(LogEntryHeader) + logEntryInFlight.logEntry.header.dataLength);
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
void CRAQReplication::init() {
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
void CRAQReplication::setup(Message *message) {
    setupMessage_ = message;
}

/**
 * Handles an incoming response for a previous send out SETUP message
 * @param message Message contains important meta information/pointer e.g. Request Handle, resp/req Buffers
 */
void CRAQReplication::setup_response(Message *message) {
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
void CRAQReplication::append(Message *message) {
    DEBUG_MSG("CRAQReplication.append(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");
    /* Assumes that the HEAD only sends messages, when it received the SETUP response */
    chainReady_ = true;
    auto *reqLogEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->reqBuffer.buf);

    // FIXME: Only for benchmarking
    if (benchmarkData_.highestKnownLogOffset < reqLogEntryInFlight->header.logOffset)
        benchmarkData_.highestKnownLogOffset = reqLogEntryInFlight->header.logOffset;

    switch(nodeType_) {
        case HEAD: 
        {
            /* Count Sequencer up and set the log entry number */
            reqLogEntryInFlight->header.logOffset = softCounter_.fetch_add(1); 
            message->logOffset = reqLogEntryInFlight->header.logOffset;

            // FIXME: Only for benchmarking
            if (benchmarkData_.highestKnownLogOffset < reqLogEntryInFlight->header.logOffset)
                benchmarkData_.highestKnownLogOffset = reqLogEntryInFlight->header.logOffset;

            /* Append the log entry to the local Log */
            log_.append(message->logOffset, &reqLogEntryInFlight->logEntry);
            
            /* Send APPEND to next node in chain */
            networkManager_->send_message(SUCCESSOR, message);
        }; break;
        case MIDDLE: 
        {
            /* Append the log entry to the local log */
            log_.append(message->logOffset, &reqLogEntryInFlight->logEntry);

            /* Send APPEND to next node in chain */
            networkManager_->send_message(SUCCESSOR, message);
        }; break;
        case TAIL: 
        {
            // Set state CLEAN, since TAIL is last node in the chain
            reqLogEntryInFlight->logEntry.header.state = CLEAN;
            /* Append the log entry to the local log */
            log_.append(message->logOffset, &reqLogEntryInFlight->logEntry);
            /* Add logOffset from reqBuffer to respBuffer */
            auto *respLogEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->respBuffer.buf);
            respLogEntryInFlight->header.logOffset = message->logOffset;
            message->respBufferSize = sizeof(LogEntryInFlightHeader);

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
            DEBUG_MSG("CRAQReplication.read(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");
            DEBUG_MSG("CRAQReplication.read(reqLogEntryInFlight: logOffset: " << std::to_string(((LogEntryInFlight *) message->reqBuffer.buf)->header.logOffset) << " ; dataLength: " << std::to_string(((LogEntryInFlight *) message->reqBuffer.buf)->logEntry.header.dataLength) << " ; data: " << ((LogEntryInFlight *) message->reqBuffer.buf)->logEntry.data << ")");

    switch(nodeType_) {
        case HEAD: 
        case MIDDLE: 
        {
            // TODO: Check if logOffset < counter
            auto [logEntry, logEntryLength] = log_.read(message->logOffset);

            if (logEntryLength == 0) {
                // Entry hasn't been written yet or is written atm, so it hasn't reached the tail yet for sure
                auto *respLogEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->respBuffer.buf);
                message->respBufferSize = sizeof(LogEntryHeader) + sizeof(LogEntryInFlightHeader);
                respLogEntryInFlight->header.logOffset = message->logOffset;

                // Return length 0 for indicating entry hasn't been written yet
                respLogEntryInFlight->logEntry.header.dataLength = 0;

                if (message->sentByThisNode) {
                    this->receive_locally(message);
                    return;
                }

                /* Send READ response */
                networkManager_->send_response(message);
            } else if (logEntry->header.state == CLEAN || uncommittedRead) {
                auto *respLogEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->respBuffer.buf);

                /* Prepare respBuffer */
                message->respBufferSize = logEntryLength + sizeof(LogEntryInFlightHeader);
                respLogEntryInFlight->header.logOffset = message->logOffset;
                memcpy(&respLogEntryInFlight->logEntry, logEntry, logEntryLength);

                if (message->sentByThisNode) {
                    this->receive_locally(message);
                    return;
                }

                /* Send READ response */
                networkManager_->send_response(message);
            } else {
                auto *reqLogEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->reqBuffer.buf);
                reqLogEntryInFlight->header.messageType = GET_LOG_ENTRY_STATE;
                message->messageType = GET_LOG_ENTRY_STATE;

                /* Send GET_LOG_ENTRY_STATE request to TAIL */
                networkManager_->send_message(TAIL, message);
            }
            DEBUG_MSG("CRAQReplication.read(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");
            DEBUG_MSG("CRAQReplication.read(reqLogEntryInFlight: logOffset: " << std::to_string(((LogEntryInFlight *) message->reqBuffer.buf)->header.logOffset) << " ; dataLength: " << std::to_string(((LogEntryInFlight *) message->reqBuffer.buf)->logEntry.header.dataLength) << " ; data: " << ((LogEntryInFlight *) message->reqBuffer.buf)->logEntry.data << ")");
        }; break;
        case TAIL:
        {
            auto *respLogEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->respBuffer.buf);
            respLogEntryInFlight->header.logOffset = message->logOffset;

            auto [logEntry, logEntryLength] = log_.read(message->logOffset);
            
            // If length is 0, entry hasn't been written yet
            if (logEntryLength == 0) {
                message->respBufferSize = sizeof(LogEntryHeader) + sizeof(LogEntryInFlightHeader);
                // Return length 0 for indicating entry hasn't been written yet
                respLogEntryInFlight->logEntry.header.dataLength = 0;
            } else {
                message->respBufferSize = logEntryLength + sizeof(LogEntryInFlightHeader);
                memcpy(&respLogEntryInFlight->logEntry, logEntry, logEntryLength);
            }

            DEBUG_MSG("CRAQReplication.read(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");
    	    DEBUG_MSG("CRAQReplication.read(respLogEntryInFlight: dataLength: " << std::to_string(((LogEntryInFlight *) message->respBuffer.buf)->logEntry.header.dataLength) << " ; data: " << ((LogEntryInFlight *) message->respBuffer.buf)->logEntry.data << ")");

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
    message->respBufferSize = sizeof(LogEntryInFlightHeader) + sizeof(LogEntryHeader);
    respLogEntryInFlight->logEntry.header.state = logEntry->header.state;

    /* Send GET_LOG_ENTRY_STATE response */
    networkManager_->send_response(message);
}


void CRAQReplication::get_log_entry_state_response(Message *message) {
    auto *respLogEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->respBuffer.buf);
    if(respLogEntryInFlight->logEntry.header.state == CLEAN) {
        log_.update_logEntryState(message->logOffset, CLEAN); 

        networkManager_->rpc_.free_msg_buffer(message->respBuffer);
        // Alloc new respBuffer with MAX_MESSAGE_SIZE
        message->respBuffer = networkManager_->rpc_.alloc_msg_buffer(MAX_MESSAGE_SIZE);
        while(!message->respBuffer.buf) {
            networkManager_->rpc_.run_event_loop_once();
            message->respBuffer = networkManager_->rpc_.alloc_msg_buffer(MAX_MESSAGE_SIZE);
        }
        
        this->read(message); // FIXME: Doing another read
        benchmarkData_.amountStateRequests++;
    } else if(respLogEntryInFlight->logEntry.header.state == ERROR) {
        if (message->sentByThisNode) {
            this->receive_locally(message);
            return;
        }
        networkManager_->send_response(message);
    }
}


void CRAQReplication::terminate(Message *message) {
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

void CRAQReplication::terminate_response(Message *message) {
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
void CRAQReplication::receive_locally(Message *message) {
    benchmarkData_.totalMessagesProcessed++;
    
    if (message->messageType == APPEND) {
        benchmarkData_.amountAppendsSent++; 
        
        #ifdef LATENCY
        double req_lat_us = erpc::to_usec(erpc::rdtsc() - message->timestamp, networkManager_->rpc_.get_freq_ghz());
        benchmarkData_.appendlatency.update(static_cast<size_t>(req_lat_us * benchmarkData_.latencyFactor));
        #endif
    } else if(message->messageType == READ) {
        benchmarkData_.amountReadsSent++;

        // count error read messages
        auto *respLogEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->respBuffer.buf);
        if (respLogEntryInFlight->logEntry.header.dataLength == 0)
            benchmarkData_.amountReadsErrors++;
    } else if (message->messageType == GET_LOG_ENTRY_STATE) {
        benchmarkData_.amountReadsSent++;
        benchmarkData_.amountStateRequests++;
    }

    networkManager_->rpc_.free_msg_buffer(message->reqBuffer);
    networkManager_->rpc_.free_msg_buffer(message->respBuffer);
    delete message;
}


/**
 * Terminates the current CRAQReplication thread
 * @param force If true, forces the thread to finish
 */
void CRAQReplication::join(bool force) {
    if (force)
        threadSync_.threadReady = false;

    thread_.join();
}
