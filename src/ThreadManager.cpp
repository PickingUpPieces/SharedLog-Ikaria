#include "ThreadManager.h"

/* TODO: Documentation */
/**
 * Constructs the ThreadManager as single threaded
 * @param NodeType Specifys the type of this node (HEAD, MIDDLE or TAIL)
 * @param headURI String "hostname:port" of the HEAD node of the chain. If this node is the HEAD, leave it empty.
 * @param successorURI String "hostname:port" of the SUCCESSOR node of this node in the chain.
 * @param tailURI String "hostname:port" of the TAIL node of the chain. If this node is the TAIL, leave it empty.
 * @param rec Callback function which is called when a message response is received which has been created by this node
 */
ThreadManager::ThreadManager(NodeType nodeType, uint8_t nodeID, const char* pathToLog, erpc::Nexus *nexus, string headURI, string successorURI, string tailURI, receive_local rec):
        nodeID_{nodeID},
        replicationManager_{new ReplicationManager(nodeType, pathToLog, nexus, headURI, successorURI, tailURI, rec)},
        threadSync_{},
        nodeType_{nodeType}
    {
        threadSync_.threadReady = true; 
    } 


/* TODO: Documentation */
/**
 * Constructs the ThreadManager as multi threaded Object
 * @param headURI String "hostname:port" of the HEAD node of the chain. If this node is the HEAD, leave it empty.
 * @param successorURI String "hostname:port" of the SUCCESSOR node of this node in the chain.
 * @param tailURI String "hostname:port" of the TAIL node of the chain. If this node is the TAIL, leave it empty.
 * @param rec Callback function which is called when a message response is received which has been created by this node
*/ 
ThreadManager::ThreadManager(NodeType nodeType, uint8_t nodeID, const char* pathToLog, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI, BenchmarkData benchmarkData): 
        nodeID_{nodeID},
        replicationManager_{nullptr},
        benchmarkData_{benchmarkData},
        nodeType_{nodeType},
        rec_{nullptr} 
    {
        if (benchmarkData_.progArgs.activeMode)
            thread_ = std::thread(run_active, this, pathToLog, nexus, erpcID, headURI, successorURI, tailURI); 
        else
            thread_ = std::thread(run_passive, this, pathToLog, nexus, erpcID, headURI, successorURI, tailURI); 
    }


/* TODO: Documentation */
/* Active Thread function */
void ThreadManager::run_active(ThreadManager *tm, const char* pathToLog, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI) {
    tm->replicationManager_ = new ReplicationManager(tm->nodeType_, pathToLog, nexus, headURI, successorURI, tailURI, nullptr);
    tm->replicationManager_->init();

    auto logEntryInFlight = generate_random_logEntryInFlight(tm->benchmarkData_.progArgs.valueSize);

    // Append few messages so something can be read
    for(int i = 0; i < 100; i++) 
        append(tm, &logEntryInFlight, logEntryInFlight.logEntry.dataLength + (2 * 8) + sizeof(MessageType));

    while(tm->replicationManager_->networkManager_->messagesInFlight_)
		tm->replicationManager_->networkManager_->sync(1);

    // Set threadReady to true
    unique_lock<mutex> lk(tm->threadSync_.m);
    tm->threadSync_.threadReady = true;
    lk.unlock();
    tm->threadSync_.cv.notify_all();

    // Start threads (more or less) simultaniously 
    tm->benchmarkData_.startBenchmark->lock();
    tm->benchmarkData_.startBenchmark->unlock();

    while(likely(tm->threadSync_.threadReady && tm->benchmarkData_.remainderNumberOfRequests)) {
        if (( rand() % 100 ) < tm->benchmarkData_.progArgs.probabilityOfRead) {
	        if ( tm->benchmarkData_.highestKnownLogOffset < 1)
		        continue;

	        auto randuint = static_cast<uint64_t>(rand());
            auto randReadOffset = randuint % tm->benchmarkData_.highestKnownLogOffset; 
            logEntryInFlight.messageType = READ;
            read(tm, randReadOffset);
	        tm->benchmarkData_.amountReadsSent++; 
        } else {
            append(tm, &logEntryInFlight, logEntryInFlight.logEntry.dataLength + (2 * 8) + sizeof(MessageType));
	        tm->benchmarkData_.amountAppendsSent++; 
        }
	    tm->benchmarkData_.remainderNumberOfRequests--; 
    }

    /* Wait for missing response messages */
    while((tm->benchmarkData_.progArgs.totalNumberOfRequests - tm->benchmarkData_.messagesInFlight) < (tm->benchmarkData_.progArgs.totalNumberOfRequests - tm->benchmarkData_.progArgs.percentileNumberOfRequests))
		tm->replicationManager_->networkManager_->sync(1);
    tm->benchmarkData_.totalMessagesProcessed = tm->replicationManager_->networkManager_->totalMessagesProcessed_;
}

/* TODO: Documentation */
/* Passive Thread function */
void ThreadManager::run_passive(ThreadManager *tm, const char* pathToLog, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI) {
    tm->replicationManager_->init();

    // Set threadReady to true
    unique_lock<mutex> lk(tm->threadSync_.m);
    tm->threadSync_.threadReady = true;
    lk.unlock();
    tm->threadSync_.cv.notify_all();

    if (tm->nodeType_ == HEAD)
        read(tm, 0);

    while(likely(tm->threadSync_.threadReady))
		tm->replicationManager_->networkManager_->sync(1);

    tm->benchmarkData_.totalMessagesProcessed = tm->replicationManager_->networkManager_->totalMessagesProcessed_;
}

/* TODO: Documentation */
/* readLog method */
void ThreadManager::read(ThreadManager *tm, uint64_t logOffset) {
    /* Allocate message struct */
    Message *message = (Message *) malloc(sizeof(Message));

    message->reqBuffer = tm->replicationManager_->networkManager_->rpc_.alloc_msg_buffer(MAX_MESSAGE_SIZE);
    while(!message->reqBuffer.buf) {
        tm->replicationManager_->networkManager_->sync(1);
        message->reqBuffer = tm->replicationManager_->networkManager_->rpc_.alloc_msg_buffer(MAX_MESSAGE_SIZE);
    }

    message->respBuffer = tm->replicationManager_->networkManager_->rpc_.alloc_msg_buffer(MAX_MESSAGE_SIZE);
    while(!message->respBuffer.buf) {
        tm->replicationManager_->networkManager_->sync(1);
        message->respBuffer = tm->replicationManager_->networkManager_->rpc_.alloc_msg_buffer(MAX_MESSAGE_SIZE);
    }

    /* Fill message struct */
	message->respBufferSize = MAX_MESSAGE_SIZE;
    message->sentByThisNode = true;
    message->logOffset = logOffset;
    message->messageType = READ;

    /* Fill request data */
    auto logEntryInFlight = reinterpret_cast<LogEntryInFlight *>(message->reqBuffer.buf);
    logEntryInFlight->logOffset = message->logOffset;
    logEntryInFlight->messageType = message->messageType;
    message->reqBufferSize = 8 + sizeof(logEntryInFlight->messageType);

    /* Send the message */
    if (tm->nodeType_ == HEAD)
        tm->replicationManager_->read(message);
    else 
        tm->replicationManager_->networkManager_->send_message(HEAD, message);
}

/* TODO: Documentation */
void ThreadManager::append(ThreadManager *tm, void *data, size_t dataLength) {
    /* Allocate message struct */
    Message *message = (Message *) malloc(sizeof(Message));
    auto *logEntryInFlight = static_cast<LogEntryInFlight *>(data);
    logEntryInFlight->messageType = APPEND;

    message->reqBuffer = tm->replicationManager_->networkManager_->rpc_.alloc_msg_buffer(MAX_MESSAGE_SIZE);
    while(!message->reqBuffer.buf) {
        tm->replicationManager_->networkManager_->sync(1);
        message->reqBuffer = tm->replicationManager_->networkManager_->rpc_.alloc_msg_buffer(MAX_MESSAGE_SIZE);
    }

    message->respBuffer = tm->replicationManager_->networkManager_->rpc_.alloc_msg_buffer(MAX_MESSAGE_SIZE);
    while(!message->respBuffer.buf) {
        tm->replicationManager_->networkManager_->sync(1);
        message->respBuffer = tm->replicationManager_->networkManager_->rpc_.alloc_msg_buffer(MAX_MESSAGE_SIZE);
    }

    /* Fill message struct */
    message->reqBufferSize = MAX_MESSAGE_SIZE;
	message->respBufferSize = MAX_MESSAGE_SIZE;
    message->sentByThisNode = true;
    message->messageType = APPEND;

    /* Fill request data */
    memcpy(message->reqBuffer.buf, data, dataLength);
    message->reqBufferSize = dataLength;

    DEBUG_MSG("sharedLogNode.append(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");

    /* Send the message */
    if (tm->nodeType_ == HEAD)
        tm->replicationManager_->append(message);
    else 
        tm->replicationManager_->networkManager_->send_message(HEAD, message);
} 

/* Generate a random logEntryInFlight for sending in append requests */
LogEntryInFlight ThreadManager::generate_random_logEntryInFlight(uint64_t totalSize){
    LogEntryInFlight logEntryInFlight;
    string possibleCharacters = "123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    mt19937 generator{random_device{}()};
    uniform_int_distribution<> dist(0, possibleCharacters.size()-1);
    string randomString;
    uint64_t stringLength = totalSize - 16;

    for(uint64_t i = 0; i < stringLength; i++) {
        size_t random_index = static_cast<size_t>(dist(generator)); //get index between 0 and possible_characters.size()-1
        randomString += possibleCharacters[random_index];
    }

    logEntryInFlight.logOffset = 0; 
    logEntryInFlight.logEntry.dataLength = stringLength;
    randomString.copy(logEntryInFlight.logEntry.data, randomString.length());

    return logEntryInFlight;
}

/* TODO: Documentation */
/* Callback function when a response is received */
void ThreadManager::default_receive_local(Message *message) {
    // If single threaded
    if (rec_) { 
        rec_(message);
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
void ThreadManager::terminate(bool force) {
    if (force)
        threadSync_.threadReady = false;

    thread_.join();
}