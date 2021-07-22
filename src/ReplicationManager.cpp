#include <iostream>
#include <unistd.h>
#include "ReplicationManager.h"

/* Init static softCounter */
static std::atomic<uint64_t> softCounter_{0}; 


void appendLog(ReplicationManager *rp, void *data, size_t dataLength);
void readLog(ReplicationManager *rp, uint64_t logOffset);
//#define REPLMAN_HEAD

/**
 * Constructs the ReplicationManager 
 * @param NodeType Specifys the type of this node (HEAD, MIDDLE or TAIL)
 * @param headURI String "hostname:port" of the HEAD node of the chain. If this node is the HEAD, leave it empty.
 * @param successorURI String "hostname:port" of the SUCCESSOR node of this node in the chain.
 * @param tailURI String "hostname:port" of the TAIL node of the chain. If this node is the TAIL, leave it empty.
 * @param rec Callback function which is called when a message response is received which has been created by this node
*/ ReplicationManager::ReplicationManager(NodeType NodeType, erpc::Nexus *Nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI, bool runAsThread, receive_local rec): 
        nodeReady_{false},
        setupMessage_{nullptr},
        Log_{POOL_SIZE, LOG_BLOCK_TOTAL_SIZE, POOL_PATH}, 
        chainReady_{false},
        NodeType_{NodeType},
        rec{rec}
    {
        if (runAsThread)
            thread_ = std::thread(run, this, Nexus, erpcID, headURI, successorURI, tailURI); 
        else
            NetworkManager_ = new NetworkManager(Nexus, erpcID, headURI, successorURI, tailURI, this); 
    }


/* TODO: Documentation */
/* Thread function */
void ReplicationManager::run(ReplicationManager *rp, erpc::Nexus *Nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI) {
    rp->NetworkManager_ = new NetworkManager(Nexus, erpcID, headURI, successorURI, tailURI, rp); 
    rp->init();
    DEBUG_MSG("ReplicationManager.run(SETUP done)");

    LogEntryInFlight logEntryInFlight{1, { 0, ""}};
    string randomString = "help";
    randomString.copy(logEntryInFlight.logEntry.data, randomString.length());
    logEntryInFlight.logEntry.dataLength = randomString.length();

    while(likely(rp->nodeReady_)) {
        appendLog(rp, &logEntryInFlight, logEntryInFlight.logEntry.dataLength + (2 * 8));
        readLog(rp, 1);

	while (rp->NetworkManager_->messagesInFlight_ > 10000)
		rp->NetworkManager_->sync(10);
    }
}

/**
 * Handles the SETUP process for this node
*/
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
    	DEBUG_MSG("ReplicationManager.init(Wait for Setup)");
        /* Wait for the SETUP message */
        while (!setupMessage_)
            NetworkManager_->sync(10);

    	DEBUG_MSG("ReplicationManager.init(Setup arrived)");
        /* Answer/Forward SETUP message accordingly */
        if (NodeType_ == TAIL)
            NetworkManager_->send_response(setupMessage_);
        else
            NetworkManager_->send_message(SUCCESSOR, setupMessage_);

    	DEBUG_MSG("ReplicationManager.init(Wait for first APPEND)");
        /* Wait for first APPEND/READ message from the HEAD node */
        while (!chainReady_) 
            NetworkManager_->sync(10);
    }
}

/**
 * Handles an incoming SETUP message
 * @param message Message contains important meta information/pointer e.g. Request Handle, resp/req Buffers
 */
void ReplicationManager::setup(Message *message) {
    DEBUG_MSG("ReplicationManager.setup()");
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
            LogEntryInFlight *reqLogEntryInFlight = (LogEntryInFlight *) message->reqBuffer->buf;
            /* Count Sequencer up and set the log entry number */
            reqLogEntryInFlight->logOffset = softCounter_.fetch_add(1); // FIXME: Check memory relaxation of fetch_add

            #ifdef TESTING
            add_logOffset_to_data(message);
            #endif 

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
            message->logOffset = ((LogEntryInFlight *) message->reqBuffer->buf)->logOffset;
            #endif
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

/* TODO: Documentation */
/* TODO: Merge those together */
void ReplicationManager::terminate() {
    nodeReady_ = false;
    thread_.join();
}

/* TODO: Documentation */
void ReplicationManager::join_thread() {
    thread_.join();
}

/* DEBUG functions */
/**
 * Adds the logOffset number to the data, so it can be validated later if the right entries are written in the write logOffsets.
 * @param message Message contains important meta information/pointer e.g. Request Handle, resp/req Buffers
 */
void ReplicationManager::add_logOffset_to_data(Message *message) {
    LogEntryInFlight *reqLogEntryInFlight = (LogEntryInFlight *) message->reqBuffer->buf;
    string temp = (string) reqLogEntryInFlight->logEntry.data;
    temp += "-ID-" + std::to_string(reqLogEntryInFlight->logOffset);
    temp.copy(reqLogEntryInFlight->logEntry.data, temp.length());
    reqLogEntryInFlight->logEntry.dataLength = temp.length() + 1;
    message->logOffset = reqLogEntryInFlight->logOffset;
    message->reqBufferSize = sizeof(reqLogEntryInFlight->logOffset) + sizeof(reqLogEntryInFlight->logEntry.dataLength) + reqLogEntryInFlight->logEntry.dataLength;
    NetworkManager_->rpc_.resize_msg_buffer(message->reqBuffer, message->reqBufferSize);
}


/* TODO: Documentation */
/* readLog method */
void readLog(ReplicationManager *rp, uint64_t logOffset) {
    /* Allocate message struct */
    Message *message = (Message *) malloc(sizeof(Message));
    erpc::MsgBuffer *reqRead = (erpc::MsgBuffer *) malloc(sizeof(erpc::MsgBuffer));
    *reqRead = rp->NetworkManager_->rpc_.alloc_msg_buffer_or_die(MAX_MESSAGE_SIZE);

    /* Fill message struct */
    message->reqBuffer = reqRead;
	message->respBuffer = rp->NetworkManager_->rpc_.alloc_msg_buffer_or_die(MAX_MESSAGE_SIZE);
	message->respBufferSize = MAX_MESSAGE_SIZE;
    message->sentByThisNode = true;
    message->logOffset = logOffset;
    message->messageType = READ;

    /* Fill request data */
    uint64_t *reqPointer = (uint64_t *) message->reqBuffer->buf;
    *reqPointer = message->logOffset;
    message->reqBufferSize = sizeof(uint64_t);

    /* WORKAROUND resizing problem */
    #if REPLMAN_HEAD
        rp->NetworkManager_->rpc_.resize_msg_buffer(message->reqBuffer, message->reqBufferSize);
    #else
    	if (message->reqBufferSize < 969)
    	    rp->NetworkManager_->rpc_.resize_msg_buffer(message->reqBuffer, 969);
    	else
    	    rp->NetworkManager_->rpc_.resize_msg_buffer(message->reqBuffer, message->reqBufferSize);
    #endif

    /* Send the message */
    if (rp->NodeType_ == HEAD)
        rp->read(message);
    else 
        rp->NetworkManager_->send_message(HEAD, message);
}

/* TODO: Documentation */
void appendLog(ReplicationManager *rp, void *data, size_t dataLength) {
    /* Allocate message struct */
    Message *message = (Message *) malloc(sizeof(Message));
    erpc::MsgBuffer *reqRead = (erpc::MsgBuffer *) malloc(sizeof(erpc::MsgBuffer));
    *reqRead = rp->NetworkManager_->rpc_.alloc_msg_buffer_or_die(MAX_MESSAGE_SIZE);

    /* Fill message struct */
    message->reqBuffer = reqRead;
    message->reqBufferSize = MAX_MESSAGE_SIZE;
	message->respBuffer = rp->NetworkManager_->rpc_.alloc_msg_buffer_or_die(MAX_MESSAGE_SIZE);
	message->respBufferSize = MAX_MESSAGE_SIZE;
    message->sentByThisNode = true;
    message->messageType = APPEND;

    /* Fill request data */
    memcpy(message->reqBuffer->buf, data, dataLength);
    message->reqBufferSize = dataLength;

    /* WORKAROUND resizing problem */
    #if REPLMAN_HEAD
        rp->NetworkManager_->rpc_.resize_msg_buffer(message->reqBuffer, message->reqBufferSize);
    #else
    	if (message->reqBufferSize < 969)
    	    rp->NetworkManager_->rpc_.resize_msg_buffer(message->reqBuffer, 969);
    	else
    	    rp->NetworkManager_->rpc_.resize_msg_buffer(message->reqBuffer, message->reqBufferSize);
    #endif

    DEBUG_MSG("sharedLogNode.append(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");

    /* Send the message */
    if (rp->NodeType_ == HEAD)
        rp->append(message);
    else 
        rp->NetworkManager_->send_message(HEAD, message);
} 
